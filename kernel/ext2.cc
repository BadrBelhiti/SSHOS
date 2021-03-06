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

    // initialize block group table
    // starts at block 1 for any block size greater than 1024
    // block size of 1024 -> starts at block 2
    uint32_t blockGroupTableAddress = get_block_size() == 1024 ? 2 * get_block_size() : get_block_size();
    this->numBlockGroups = divisionRoundUp(this->superBlock->totalBlocks, this->superBlock->blocksPerGroup);
    this->blockGroupTable = new BlockGroupDescriptor[numBlockGroups];
    this->ide->read_all(blockGroupTableAddress, numBlockGroups * 32, (char *) blockGroupTable);

    // initialize inode and block bitmaps
    this->inodeUsageBitmaps = new char*[numBlockGroups];
    this->blockUsageBitmaps = new char*[numBlockGroups];
    for (uint32_t i = 0; i < numBlockGroups; i++) {
        // initialize inode bitmap
        this->inodeUsageBitmaps[i] = new char[get_block_size()];
        this->ide->read_all(blockGroupTable[i].inodeUsageAddress * get_block_size(), get_block_size(), inodeUsageBitmaps[i]);

        // initialize block bitmap
        this->blockUsageBitmaps[i] = new char[get_block_size()];
        this->ide->read_all(blockGroupTable[i].blockUsageAddress * get_block_size(), get_block_size(), blockUsageBitmaps[i]);
    }  

    // initialize root directory inode
    this->root = new Node(get_block_size(), 2, this); // root inode number is 2
}

int Ext2::findAvailableStructure(uint32_t startingNumber, char **usageBitmaps, uint32_t structuresPerGroup) {
    uint32_t curNumber = startingNumber;
    uint32_t bytesPerBitmap = structuresPerGroup / 8;

    for (uint32_t curBlockGroup = 0; curBlockGroup < numBlockGroups; curBlockGroup++) {
        char *usageBitmap = usageBitmaps[curBlockGroup];
        for (uint32_t curByte = 0; curByte < bytesPerBitmap; curByte++) {
            // iterate over each bit
            for (int bitShift = 7; bitShift >= 0; bitShift--) {
                uint32_t curBit = (usageBitmap[curByte] >> bitShift) & 1;
                // check if structure is allocated
                if (!curBit) {
                    uint32_t bitMask = 1 << bitShift;
                    // allocate structure
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
    uint32_t allocatedBlock = findAvailableStructure(0, blockUsageBitmaps, superBlock->blocksPerGroup);
    return allocatedBlock;
}

// return inodeNumber
int Ext2::findAvailableInode() {
    uint32_t allocatedInode = findAvailableStructure(1, inodeUsageBitmaps, superBlock->inodesPerGroup);
    return allocatedInode;
}

void Ext2::freeStructure(uint32_t number, char **usageBitmaps, uint32_t structuresPerGroup) {
    uint32_t groupNumber = number / structuresPerGroup;
    uint32_t groupBitNumber = number % structuresPerGroup;
    uint32_t byteIndex = groupBitNumber / 8;
    uint32_t bitIndex = groupBitNumber % 8;

    // free the desired bit by masking it out while preserving the rest of the byte
    char *usageBitmap = usageBitmaps[groupNumber];
    uint32_t bitMask = ~(1 << (7 - bitIndex));
    usageBitmap[byteIndex] &= bitMask;
}

void Ext2::freeBlock(uint32_t blockNumber) {
    return freeStructure(blockNumber, blockUsageBitmaps, superBlock->blocksPerGroup);
} 

void Ext2::freeInode(uint32_t inodeNumber) {
    return freeStructure(inodeNumber - 1, inodeUsageBitmaps, superBlock->inodesPerGroup);
}

void Ext2::createInode(uint16_t fileType, int inodeNumber) {
    if (inodeNumber < 1) {
        Debug::panic("Invalid inode number!");
    }

    char buffer[128];
    char *inodeData = buffer;

    // read in types and permissions
    *((uint16_t *) inodeData) = fileType;
    inodeData += 4;

    // read in inode size
    *((uint32_t *) inodeData) = 0;
    inodeData += 22;

    // read in numHardLinks - should be one as we are creating brand new file
    *((uint16_t *) inodeData) = 1;
    inodeData += 14;

    // zero out all block addresses - mark as invalid for empty inode
    for (uint32_t blockIndex = 0; blockIndex < 15; blockIndex++) {
        ((uint32_t *) inodeData)[blockIndex] = 0;
    }

    write_all(getInodeTableOffset(inodeNumber), buffer, 128);
}

void createDirectoryEntry(const char* name, int inodeNumber, uint8_t typeIndicator, Shared<Node> dir) {
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
}

// make it so zerored out directory entries just have inode number of zero
bool Ext2::createNode(Shared<Node> dir, const char* name, uint8_t typeIndicator) {
    int inodeNumber = findAvailableInode();
    // we ran out of inodes!
    if (inodeNumber == -1) {
        return false;
    }

    uint16_t fileType = 0x4000; // directory type
    if (typeIndicator == ENTRY_FILE_TYPE) { // regular file type
        fileType = 0x8000;
    } else if (typeIndicator == ENTRY_SOFT_LINK) { // soft link file type
        fileType = 0xA000;
    }

    createInode(fileType, inodeNumber);
    createDirectoryEntry(name, inodeNumber, typeIndicator, dir);

    if (typeIndicator == ENTRY_DIRECTORY_TYPE) {
        Shared<Node> newNode = get_node(inodeNumber  );
        char buffer[3];
        char *fileName = buffer;
        fileName[0] = '.';
        fileName[1] = 0;
        createDirectoryEntry(fileName, inodeNumber, ENTRY_DIRECTORY_TYPE, newNode); // creating . entry
        fileName[1] = '.';
        fileName[2] = 0;
        createDirectoryEntry(fileName, dir->number, ENTRY_DIRECTORY_TYPE, newNode); // creating .. entry
    }

    return true;
}

Shared<Node> Ext2::get_node(uint32_t number) {
    // ASSERT(number > 0);
    // ASSERT(number <= superBlock->inodesPerGroup);
    // auto index = number - 1;

    // auto groupIndex = index / superBlock->inodesPerGroup;
    // //Debug::printf("groupIndex %d\n",groupIndex);
    // ASSERT(groupIndex < numBlockGroups);
    // auto indexInGroup = index % superBlock->inodesPerGroup;
    // auto iTableBase = blockGroupTable[groupIndex].inodeTableAddress;
    // // ASSERT(iTableBase <= numberOfBlocks);
    // auto nodeOffset = iTableBase * get_block_size() + indexInGroup * get_inode_size();

    // Debug::printf("I made it here bro\n");
    // auto out = Shared<Node>::make(get_block_size(), number, this);
    // ide->read_all(nodeOffset, 128, (char *) out->inode);
    Node *node = new Node(get_block_size(), number, this);
    return Shared<Node>{node};
    // return out;
}

// If the given node is a directory, return a reference to the
// node linked to that name in the directory.
//
// Returns a null reference if "name" doesn't exist in the directory
//
// Panics if "dir" is not a directory
Shared<Node> Ext2::find(Shared<Node> current, const char* path) {
    auto part = new char[257];
    uint32_t idx = 0;

     while (true) {
        while (path[idx] == '/') idx++;
        if ((current == nullptr) || (path[idx] == 0)) {
            goto done;
        }
        uint32_t i = 0;
        while (true) {
            auto c = path[idx];

            if ((c == 0) || (c == '/')) break;
            idx ++;
            ASSERT(i < 256);
            part[i++] = c;
        }
        part[i] = 0;
        auto number = current->find(part);
        if (number == 0) {
            current = Shared<Node>{};
            goto done;
        } else {
            current = get_node(number);
        }
    }

     done:
        delete[] part;
        return current;
}

uint32_t Node::find(const char* name) {
    uint32_t out = 0;

    entries([&out,name](uint32_t number, const char* nm) {
        if (K::streq(name,nm)) {
            out = number;
        }
    });

    return out;
}


char *Node::get_entry_names(char* buff_start, uint32_t max_size) {
    ASSERT(is_dir());

    uint32_t byte = 0;
    entries([&byte, buff_start](uint32_t, char* entry_name) {
        K::strcpy(buff_start + byte, entry_name);
        byte += K::strlen(entry_name) + 1; // increment by name length, +1 for null
    });

    buff_start[byte] = '\0'; // to indicate the end of the list

    return buff_start;
}