#ifndef _ext2_h_
#define _ext2_h_

#include "ide.h"
#include "shared.h"
#include "atomic.h"
#include "debug.h"
#include "libk.h"

#define ENTRY_FILE_TYPE 1
#define ENTRY_DIRECTORY_TYPE 2
#define ENTRY_SOFT_LINK 7

struct SuperBlock {
    uint32_t totalInodes;
    uint32_t totalBlocks;
    uint8_t notNeeded[16];
    uint32_t blockSizeShift;
    uint8_t notNeeded2[4];
    uint32_t blocksPerGroup;
    uint8_t notNeeded3[4];
    uint32_t inodesPerGroup;
    uint8_t notNeeded4[980];
};

struct BlockGroupDescriptor {
    uint32_t blockUsageAddress;
    uint32_t inodeUsageAddress;
    uint32_t inodeTableAddress;
    uint16_t availableBlocks;
    uint16_t availableInodes;
    uint8_t notNeeded2[16];
};

struct Inode {
    uint16_t typesAndPermissions;
    uint8_t notNeeded[2];
    uint32_t sizeInBytes;
    uint8_t notNeeded2[18];
    uint16_t numHardLinks;
    uint8_t notNeeded3[12];
    uint32_t blockAddresses[15];
    uint8_t notNeeded4[28];
};

class Node;

// This class encapsulates the implementation of the Ext2 file system
class Ext2 {
private:
    // The device on which the file system resides
    Atomic<uint32_t> ref_count{0};

    // allocate first available structure
    int findAvailableStructure(uint32_t startingNumber, char **usageBitmaps, uint32_t structuresPerGroup);
    
    // delete directory entry from dir
    void deleteDirectoryEntry(Shared<Node> dir, uint32_t numberToDelete);

    void freeStructure(uint32_t number, char **usageBitmaps, uint32_t structuresPerGroup);

    void createInode(uint16_t fileType, int inodeNumber);

public:
    Shared<Node> root; // The root directory for this file system
    Shared<Ide> ide;
    SuperBlock *superBlock;
    uint32_t numBlockGroups;
    BlockGroupDescriptor *blockGroupTable;
    char **inodeUsageBitmaps;
    char **blockUsageBitmaps;

public:
    // Mount an existing file system residing on the given device
    // Panics if the file system is invalid
    Ext2(Shared<Ide> ide);

    // Returns the block size of the file system. Doesn't have
    // to match that of the underlying device
    uint32_t get_block_size() {
        // return 1024 << *((uint32_t *) (this->superBlock + 24));
        return 1024 << superBlock->blockSizeShift;
    }

    // Returns the actual size of an i-node. Ext2 specifies that
    // an i-node will have a minimum size of 128B but could have
    // more bytes for extended attributes
    uint32_t get_inode_size() {
        return 128;
    }

    // allocate first available block
    int findAvailableBlock();
    
    // allocate first available inode
    int findAvailableInode();

    void freeBlock(uint32_t blockNumber);

    void freeInode(uint32_t inodeNumber);

    Shared<Node> get_node(uint32_t number);

    uint32_t getInodeTableOffset(uint32_t inodeNumber) {
        uint32_t blockGroup = (inodeNumber - 1) / superBlock->inodesPerGroup;
        uint32_t inodeIndex = (inodeNumber - 1) % superBlock->inodesPerGroup;

        uint32_t byteInodeTableAddress = blockGroupTable[blockGroup].inodeTableAddress * get_block_size();
        uint32_t inodeOffset = byteInodeTableAddress + inodeIndex * 128;

        return inodeOffset;
    }

    void write_all(uint32_t diskOffset, char *bufferToWrite, uint32_t bytesToWrite) {
        // write to blocks
        int remainingBytes = bytesToWrite;
        uint32_t curOffset = diskOffset;
        while (remainingBytes > 0) {
            uint32_t writeCount = ide->write(curOffset, bufferToWrite, remainingBytes);
            bufferToWrite += writeCount;
            curOffset += writeCount;
            remainingBytes -= writeCount;
        }
    }

    bool createNode(Shared<Node> dir, const char* name, uint8_t typeIndicator);

    // If the given node is a directory, return a reference to the
    // node linked to that name in the directory.
    //
    // Returns a null reference if "name" doesn't exist in the directory
    //
    // Panics if "dir" is not a directory
    Shared<Node> find(Shared<Node> dir, const char* name);

    friend class Shared<Ext2>;
};

// A wrapper around an i-node
class Node : public BlockIO { // we implement BlockIO because we
                              // represent data
    Atomic<uint32_t> ref_count{0};

public:
    const uint32_t number; // i-number of this node
    Inode *inode;
    uint32_t type;
    Ext2 *fileSystem;

    Node(uint32_t block_size, uint32_t number, Ext2 *fileSystem) : BlockIO(block_size), number(number) {
        uint32_t blockGroup = (number - 1) / fileSystem->superBlock->inodesPerGroup;
        uint32_t inodeIndex = (number - 1) % fileSystem->superBlock->inodesPerGroup;

        char *inode = new char[128];
        uint32_t byteInodeTableAddress = fileSystem->blockGroupTable[blockGroup].inodeTableAddress * block_size;
        uint32_t inodeOffset = byteInodeTableAddress + inodeIndex * 128;

        fileSystem->ide->read_all(inodeOffset, 128, inode);

        this->inode = (Inode *) inode;
        this->type = this->inode->typesAndPermissions & 0xF000;
        this->fileSystem = fileSystem;
    }

    virtual ~Node() {}

    Shared<Node> get_node(uint32_t number);

    // How many bytes does this i-node represent
    //    - for a file, the size of the file
    //    - for a directory, implementation dependent
    //    - for a symbolic link, the length of the name
    uint32_t size_in_bytes() override {
        return inode->sizeInBytes;
    }

    // read the given block (panics if the block number is not valid)
    // remember that block size is defined by the file system not the device
    void read_block(uint32_t blockNumber, char* buffer) override {
        if (blockNumber < 12) { // direct case
            uint32_t blockAddress = inode->blockAddresses[blockNumber];
            fileSystem->ide->read_all(blockAddress * block_size, block_size, buffer);
        } else { // singly indirect case
            uint32_t blockAddress = inode->blockAddresses[12];
            fileSystem->ide->read_all(blockAddress * block_size, block_size, buffer);
            blockNumber -= 12;
            blockAddress = ((uint32_t *) buffer)[blockNumber];
            fileSystem->ide->read_all(blockAddress * block_size, block_size, buffer);
        }
    }

    // when writing directory entries, we need to first zero out the remainder of a block if 
    // there isn't enough remaining space for the new entry
    void write_all(uint32_t fileOffset, char *bufferToWrite, uint32_t bytesToWrite) {
        if (fileOffset > size_in_bytes()) {
            Debug::panic("PANIC: FileOffset requested to write is greater than file size\n");
        }

        // write to file
        int remainingBytes = bytesToWrite;
        uint32_t curOffset = fileOffset;
        while (remainingBytes > 0) {
            uint32_t blockNumber = curOffset / block_size;
            uint32_t blockOffset = curOffset % block_size;

            if (blockNumber < 12) {
                // invalid blockAddress. We need to allocate a new block for this node
                if (inode->blockAddresses[blockNumber] == 0) {
                    inode->blockAddresses[blockNumber] = fileSystem->findAvailableBlock();
                }
                uint32_t blockAddress = inode->blockAddresses[blockNumber];
                uint32_t writeAddress = blockAddress * block_size + blockOffset;

                uint32_t writeCount = fileSystem->ide->write(writeAddress, bufferToWrite, remainingBytes);
                curOffset += writeCount;
                bufferToWrite += writeCount;
                remainingBytes -= writeCount;
            } else {
                Debug::panic("PANIC: Haven't handled writing indirection yet\n");
            }
        }

        // update file size
        int addedBytes = (fileOffset + bytesToWrite) - inode->sizeInBytes;
        if (addedBytes > 0) {
            inode->sizeInBytes += addedBytes;
        }
        
        // update inode table with new size and other info
        fileSystem->write_all(fileSystem->getInodeTableOffset(number), (char *) inode, fileSystem->get_inode_size());
    }

    void deleteFromDirectory(uint32_t inodeToDelete) {
        char initBuffer[inode->sizeInBytes];
        char *buffer = initBuffer;
        read_all(0, inode->sizeInBytes, buffer);

        uint32_t curByte = 0;

        while (curByte < inode->sizeInBytes) {
            uint32_t inodeNumber = *((uint32_t *) buffer);
            uint32_t entrySize = *((uint16_t *) (buffer + 4));
            // zero out this directory entry (make it a dummy entry)
            if (inodeNumber == inodeToDelete) {
                char zeroEntry[entrySize];
                // zero out first 4 bytes
                for (uint32_t i = 0; i < 4; i++) {
                    zeroEntry[i] = 0;
                }

                // add in entry size
                *((uint16_t *) (zeroEntry + 4)) = entrySize;

                // zero out remaining bytes
                for (uint32_t i = 6; i < entrySize; i++) {
                    zeroEntry[i] = 0;
                }
                
                write_all(curByte, zeroEntry, entrySize);
                break;
            } else {
                curByte += entrySize;
                buffer += entrySize;
            }
        }
    }

    // only works for direct blocks currently
    void deleteNode(Shared<Node> parentDirectory) {  
        if (is_dir()) {
            uint32_t curByte = 0;
            Debug::printf("size in bytes: %d\n", size_in_bytes());
            while (curByte < size_in_bytes()) {
                uint32_t inodeNumber;
                read(curByte, inodeNumber);
                // don't delete dummy entries or . and .. entries
                if (inodeNumber != 0 && inodeNumber != number) {
                    Shared<Node> childNode = fileSystem->get_node(inodeNumber);
                    childNode->deleteNode(Shared<Node>{this});
                    Debug::printf("deleting node: %d\n", inodeNumber);
                }
                uint32_t entrySize;
                read(curByte + 4, entrySize);
        
                curByte += entrySize;
                break;
            }
        }

        // delete all data blocks associated with inode
        if (!(is_symlink() && inode->sizeInBytes < 60)) {
            for (uint32_t i = 0; i < 15; i++) {
                // invalid address. no more blocks to free
                if (inode->blockAddresses[i] == 0) {
                    break;
                } else { // free up block
                    fileSystem->freeBlock(inode->blockAddresses[i]);
                }
            }
        }

        // free my own inode self
        parentDirectory->deleteFromDirectory(number);
        fileSystem->freeInode(number);
    }

    // returns the ext2 type of the node
    uint32_t get_type() {
        return type;
    }

    // true if this node is a directory
    bool is_dir() {
        return type == 0x4000;
    }

    // true if this node is a file
    bool is_file() {
        return type == 0x8000;
    }

    // true if this node is a symbolic link
    bool is_symlink() {
        return type == 0xA000;
    }

    template <typename Work>
    void entries(Work work) {
        ASSERT(is_dir());
        uint32_t offset = 0;

        while (offset < inode->sizeInBytes) {
            uint32_t inode;
            read(offset,inode);
            uint16_t total_size;
            read(offset+4,total_size);
            if (inode != 0) {
                uint8_t name_length;
                read(offset+6,name_length);
                auto name = new char[name_length+1];
                name[name_length] = 0;
                auto cnt = read_all(offset+8,name_length,name);
                ASSERT(cnt == name_length);
                work(inode,name);
                delete[] name;
            }
            offset += total_size;
        }
    }

    // If this node is a symbolic link, fill the buffer with
    // the name the link refers to.
    //
    // Panics if the node is not a symbolic link
    //
    // The buffer needs to be at least as big as the the value
    // returned by size_in_bytes()
    void get_symbol(char* buffer) {
        // we have to use blocks to fetch the name
        if (inode->sizeInBytes > 60) {
            read_all(0, inode->sizeInBytes, buffer);
        } else {
            for (uint32_t i = 0; i < inode->sizeInBytes; i++) {
                buffer[i] = ((char *) inode->blockAddresses)[i];
            }
            buffer[inode->sizeInBytes] = 0;
        }
    }

    // Returns the number of hard links to this node
    uint32_t n_links() {
        return inode->numHardLinks;
    }

    // Returns the number of entries in a directory node
    //
    // Panics if not a directory
    uint32_t entry_count() {
        uint32_t entryCount = 0;
        char *buffer = new char[inode->sizeInBytes];
        char *initBuffer = buffer;
        read_all(0, inode->sizeInBytes, buffer);

        uint32_t curByte = 0;
        while (curByte < inode->sizeInBytes) {
            uint32_t inodeNumber = *((uint32_t *) buffer);
            uint32_t entrySize = *((uint16_t *) (buffer + 4));
            curByte += entrySize;
            buffer += entrySize;
            if (inodeNumber > 0) {
                entryCount++;
            }
        }
        return entryCount;

        delete[] initBuffer;
    }

    char *get_entry_names(char* buff_start, uint32_t max_size);
    uint32_t find(const char* name);

    friend class Shared<Node>;
};

#endif