#ifndef _ext2_h_
#define _ext2_h_

#include "ide.h"
#include "shared.h"
#include "atomic.h"
#include "debug.h"
#include "libk.h"

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
    uint32_t blockNumbers[15];
    uint8_t notNeeded4[28];
};

class Node;

// This class encapsulates the implementation of the Ext2 file system
class Ext2 {
    // The device on which the file system resides
    Atomic<uint32_t> ref_count{0};

public:
    Shared<Node> root; // The root directory for this file system
    Shared<Ide> ide;
    SuperBlock *superBlock;
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
        return 128; // is this right?
    }

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

    // How many bytes does this i-node represent
    //    - for a file, the size of the file
    //    - for a directory, implementation dependent
    //    - for a symbolic link, the length of the name
    uint32_t size_in_bytes() override {
        return inode->sizeInBytes;
    }

    // read the given block (panics if the block number is not valid)
    // remember that block size is defined by the file system not the device
    void read_block(uint32_t number, char* buffer) override {
        if (number < 12) { // direct case
            uint32_t blockAddress = inode->blockNumbers[number];
            fileSystem->ide->read_all(blockAddress * block_size, block_size, buffer);
        } else { // singly indirect case
            uint32_t blockAddress = inode->blockNumbers[12];
            fileSystem->ide->read_all(blockAddress * block_size, block_size, buffer);
            number -= 12;
            blockAddress = ((uint32_t *) buffer)[number];
            fileSystem->ide->read_all(blockAddress * block_size, block_size, buffer);
        }
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
                buffer[i] = ((char *) inode->blockNumbers)[i];
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
        // char *buffer = new char[inode->sizeInBytes];
        char *buffer = new char[inode->sizeInBytes];
        char *initBuffer = buffer;
        read_all(0, inode->sizeInBytes, buffer);

        uint32_t curByte = 0;
        while (curByte < inode->sizeInBytes) {
            uint32_t entrySize = *((uint16_t *) (buffer + 4));
            curByte += entrySize;
            buffer += entrySize;
            entryCount++;
        }
        return entryCount;

        delete[] initBuffer;
    }

    friend class Shared<Node>;
};

#endif
