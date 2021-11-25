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

    // initialize block group table
    // starts at block 1 for any block size greater than 1024
    // block size of 1024 -> starts at block 2
    uint32_t blockGroupTableAddress = get_block_size() == 1024 ? 2 * get_block_size() : get_block_size();
    uint32_t numBlockGroups = divisionRoundUp(this->superBlock->totalBlocks, this->superBlock->blocksPerGroup);
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

    Debug::printf("Checking inode allocations\n");
    // iterate over over every inode allocation
    uint32_t inodeNumber = 0;
    for (uint32_t i = 0; i < 1; i++) {
        Debug::printf("Inode bitmap block address: %d\n", blockGroupTable[i].blockUsageAddress);
        char *blockUsageBitmap = blockUsageBitmaps[i];
        Debug::printf("available inodes: %d\n", blockGroupTable[i].availableBlocks);
        // iterate over each byte in block
        uint32_t numBlocksFound = 0;
        for (uint32_t j = 0; j < 1024 && inodeNumber < 5120; j++) {
            // iterate over each bit
            // Debug::printf("cur byte address: %d\n", &inodeUsageBitmap[j]);
            // Debug::printf("cur byte: %d\n", inodeUsageBitmap[j]);
            // Debug::printf("curByte: %x\n", curByte);
            for (int k = 8; k >= 0; k--) {
                // allocated inode
                if ((blockUsageBitmap[j] >> k) & 1) {
                    numBlocksFound++;
                }
                inodeNumber++;
            }
        }
        Debug::printf("number of blocks found: %d\n", numBlocksFound);
    }

    // Debug::printf("Checking block allocations\n");
    // // iterate over over every block allocation
    // uint32_t blockNumber = 0;
    // for (uint32_t i = 0; i < numBlockGroups; i++) {
    //     char *blockUsageBitmap = (char *) (blockGroupTable[i].blockUsageAddress * get_block_size());
    //     Debug::printf("Block usage bitmap address: %u\n", blockUsageBitmap);

    //     // iterate over each byte in block
    //     for (uint32_t j = 0; j < 1024; j++) {
    //         // iterate over each bit
    //         uint8_t curByte = blockUsageBitmap[j];
    //         for (int k = 8; k >= 0; k--) {
    //             // allocated inode
    //             if ((curByte >> k) & 1) {
    //                 Debug::printf("found an allocated block!! block number :: %d\n", blockNumber);
    //             }
    //             blockNumber++;
    //         }
    //     }
    // }

    

    // initialize root directory inode
    this->root = new Node(get_block_size(), 2, this); // root inode number is 2
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
