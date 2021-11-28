#include "ext2.h"

uint32_t divisionRoundUp(uint32_t numerator, uint32_t denominator) { 
    return (numerator + denominator - 1) / denominator;
}

bool strEquals(const char *fileName, char *curName, uint8_t curNameLength) {
    int i = 0;
    while (i < curNameLength && fileName[i] != 0 && fileName[i] == curName[i]) {
        i++;
    }

    // reached end of both strings
    return i == curNameLength && fileName[i] == 0;
}

Ext2::Ext2(Shared<Ide> ide) {
    // initialize super block
    this->ide = ide;
    this->superBlock = new SuperBlock();

    this->ide->read_all(1024, 1024, (char *) superBlock);

    Debug::printf("total number of inodes: %d\n", superBlock->totalInodes);
    Debug::printf("total number of blocks: %d\n", superBlock->totalBlocks);

    Debug::printf("Blocks per group: %d\n", superBlock->blocksPerGroup);
     

    // initialize block group table
    // starts at block 1 for any block size greater than 1024
    // block size of 1024 -> starts at block 2
    uint32_t blockGroupTableAddress = get_block_size() == 1024 ? 2 * get_block_size() : get_block_size();
    this->numBlockGroups = divisionRoundUp(this->superBlock->totalBlocks, this->superBlock->blocksPerGroup);
    this->blockGroupTable = new BlockGroupDescriptor[numBlockGroups];
    this->ide->read_all(blockGroupTableAddress, numBlockGroups * 32, (char *) blockGroupTable);

    // initialize inode and block bitmaps
    this->inodeUsageBitmaps = new char*[numBlockGroups];
    for (uint32_t i = 0; i < numBlockGroups; i++) {
        // initialize inode bitmap
        this->inodeUsageBitmaps[i] = new char[get_block_size()];
        this->ide->read_all(blockGroupTable[i].inodeUsageAddress * get_block_size(), get_block_size(), inodeUsageBitmaps[i]);

        // initialize block bitmap
        this->blockUsageBitmaps[i] = new char[get_block_size()];
        this->ide->read_all(blockGroupTable[i].blockUsageAddress * get_block_size(), get_block_size(), blockUsageBitmaps[i]);
    }
    
    // read out number of allocated inodes for all block groups

    // Debug::printf("Checking block allocations\n");
    // // Debug::printf("number available inodes: %d\n", blockGroupTable[0].availableInodes);
    // // Debug::printf("inodes per group: %d\n", superBlock->inodesPerGroup);
    // // iterate over over every inode allocation
    // for (uint32_t i = 0; i <= 0; i++) {
    //     char *blockUsageBitmap = blockUsageBitmaps[i];
    //     // iterate over each byte in block
    //     uint32_t blockNumber = 0;
    //     for (uint32_t j = 0; j < 640; j++) {
    //         // iterate over each bit
    //         for (int k = 7; k >= 0; k--) {
    //             // allocated inode
    //             if (!(blockUsageBitmap[j] >> k) & 1) {
    //                 Debug::printf("BLOCK NUMBER Not FOUND: %u\n", blockNumber);
    //             }
    //             blockNumber++;
    //         }
    //     }
    // }    

    // initialize root directory inode
    this->root = new Node(get_block_size(), 2, this); // root inode number is 2
}

// return blockNumber
int Ext2::findAvailableStructure(uint32_t startingNumber, char **usageBitmaps, uint32_t structuresPerGroup) {
    uint32_t curNumber = startingNumber;
    uint32_t bytesPerBitmap = structuresPerGroup / 8;

    for (uint32_t curBlockGroup = 0; curBlockGroup < numBlockGroups; curBlockGroup++) {
        char *usageBitmap = usageBitmaps[curBlockGroup];
        for (uint32_t curByte = 0; curByte < bytesPerBitmap; curByte++) {
            // iterate over each bit
            for (int bitShift = 7; bitShift >= 0; bitShift--) {
                uint32_t curBit = (usageBitmap[curByte] >> bitShift) & 1;
                // check if inode is allocated
                if (!curBit) {
                    uint32_t bitMask = 1 << bitShift;
                    usageBitmap[curByte] |= bitMask;
                    // blockGroupTable[curBlockGroup].availableInodes--;
                    return curNumber;
                }
                curNumber++;
            }
        }
    }

    // we ran out of structure
    return -1;
}

int Ext2::findAvailableBlock() {
    int blockNumber = findAvailableStructure(0, blockUsageBitmaps, superBlock->blocksPerGroup);
    Debug::printf("Found block number! %d\n", blockNumber);
    return blockNumber;
}

// return inodeNumber
int Ext2::findAvailableInode() {
    return findAvailableStructure(1, inodeUsageBitmaps, superBlock->inodesPerGroup);
    // uint32_t inodeNumber = 1;
    // uint32_t bytesPerBitmap = superBlock->inodesPerGroup / 8;

    // for (uint32_t curBlockGroup = 0; curBlockGroup < numBlockGroups; curBlockGroup++) {
    //     char *inodeUsageBitmap = inodeUsageBitmaps[curBlockGroup];
    //     for (uint32_t curByte = 0; curByte < bytesPerBitmap; curByte++) {
    //         // iterate over each bit
    //         for (int bitShift = 7; bitShift >= 0; bitShift--) {
    //             uint32_t curBit = (inodeUsageBitmap[curByte] >> bitShift) & 1;
    //             // check if inode is allocated
    //             if (!curBit) {
    //                 uint32_t bitMask = 1 << bitShift;
    //                 inodeUsageBitmap[curByte] |= bitMask;
    //                 blockGroupTable[curBlockGroup].availableInodes--;
    //                 return inodeNumber;
    //             }
    //             inodeNumber++;
    //         }
    //     }
    // }

    // // we are out of inodes
    // return -1;
}

// make it so zerored out directory entries just have inode number of zero
bool Ext2::createNode(Shared<Node> dir, const char* name, uint8_t typeIndicator) {
    int inodeNumber = findAvailableInode();
    // we ran out of inodes!
    if (inodeNumber < -1) {
        return false;
    }

    Debug::printf("Found inode number: %d\n", inodeNumber);

    uint32_t nameLength = K::strlen(name);
    uint32_t directorySize = 8 + nameLength;
    // ensure directory entry is 4 byte aligned
    if (directorySize % 4 != 0) {
        directorySize += (4 - directorySize % 4);
    }

    char buffer[directorySize];
    char *directoryEntry = buffer;

    // add inodeNumber
    *((uint32_t *) directoryEntry) = inodeNumber;
    directoryEntry += 4;

    // add in entry size
    *((uint16_t *) directoryEntry) = directorySize;
    directoryEntry += 2;

    // add in name length
    *((uint8_t *) directoryEntry) = nameLength;
    directoryEntry += 1;

    // add type indicator
    *((uint8_t *) directoryEntry) = typeIndicator;
    directoryEntry += 1;

    // add in inode name
    ::memcpy(directoryEntry, name, nameLength);

    // write out directory entry to "dir"
    dir->write_all(dir->size_in_bytes(), buffer, directorySize);

    return true;
}

Shared<Node> Ext2::find(Shared<Node> dir, const char* name) {
    char *buffer = new char[dir->inode->sizeInBytes];
    dir->read_all(0, dir->inode->sizeInBytes, buffer);

    uint32_t curByte = 0;
    bool fileFound = false;
    uint32_t foundInodeNumber = 0;
    uint32_t entrySize = 0;
    uint8_t curNameLength = 0;
    while (curByte < dir->inode->sizeInBytes && !fileFound) {
        entrySize = *((uint16_t *) (buffer + 4));
        curNameLength = (uint8_t) buffer[6];
        if (strEquals(name, buffer + 8, curNameLength)) {
            foundInodeNumber = *((uint32_t *) buffer);
            fileFound = true;
        }
        curByte += entrySize;
        buffer += entrySize;
    }

    Debug::printf("foundInodeNumber: %d\n", foundInodeNumber);
    Node *foundNode = fileFound ? new Node(get_block_size(), foundInodeNumber, this) : nullptr;
    return Shared<Node>{foundNode};
}
