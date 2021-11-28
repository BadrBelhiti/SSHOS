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

    // Debug::printf("Checking inode allocations\n");
    // // iterate over over every inode allocation
    // for (uint32_t i = 0; i <= 0; i++) {
    //     char *inodeUsageBitmap = inodeUsageBitmaps[i];
    //     // iterate over each byte in block
    //     uint32_t inodeNumber = 1;
    //     for (uint32_t j = 0; j < 1024; j++) {
    //         // iterate over each bit
    //         for (int k = 7; k >= 0; k--) {
    //             // allocated inode
    //             if ((inodeUsageBitmap[j] >> k) & 1) {
    //                 Debug::printf("INODE NUMBER FOUND: %u\n", inodeNumber);
    //             }
    //             inodeNumber++;
    //         }
    //     }
    // }    

    // initialize root directory inode
    this->root = new Node(get_block_size(), 2, this); // root inode number is 2
}

// return blockNumber
uint32_t Ext2::findAvailableBlock() {
    return 0;
}

// return inodeNumber
uint32_t Ext2::findAvailableInode() {
    uint32_t inodeNumber = 1;
    uint32_t bytesPerBitmap = superBlock->inodesPerGroup / 8;
    Debug::printf("Bytes Per Bitmap: %u\n", bytesPerBitmap);

    for (uint32_t curBlockGroup = 0; curBlockGroup < numBlockGroups; curBlockGroup++) {
        char *inodeUsageBitmap = inodeUsageBitmaps[curBlockGroup];
        for (uint32_t curByte = 0; curByte < bytesPerBitmap; curByte++) {
            // iterate over each bit
            for (int bitShift = 7; bitShift >= 0; bitShift--) {
                uint32_t curBit = (inodeUsageBitmap[curByte] >> bitShift) & 1;
                // check if inode is allocated
                if (!curBit) {
                    Debug::printf("inode number found: %u\n", inodeNumber);
                    Debug::printf("Before allocating bit: %x\n", inodeUsageBitmap[curByte]);
                    uint32_t bitMask = 1 << bitShift;
                    inodeUsageBitmap[curByte] |= bitMask;
                    Debug::printf("After allocating bit: %x\n", inodeUsageBitmap[curByte]);
                    return inodeNumber;
                }
                inodeNumber++;
            }
        }
    }

    // we are out of inodes
    return -1;
}

bool Ext2::createNode(Shared<Node> dir, const char* name) {
    uint32_t inodeNumber = findAvailableInode();
    
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

    Node *foundNode = fileFound ? new Node(get_block_size(), foundInodeNumber, this) : nullptr;
    return Shared<Node>{foundNode};
}
