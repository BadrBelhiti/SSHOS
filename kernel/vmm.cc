#include "vmm.h"
#include "machine.h"
#include "idt.h"
#include "libk.h"
#include "blocking_lock.h"
#include "config.h"
#include "threads.h"
#include "debug.h"
#include "ext2.h"
#include "physmem.h"


namespace VMM {

uint32_t *globalPD;
uint32_t numIdentityPages;

void delete_page(uint32_t virtualPageNumber, uint32_t *pageDirectory) {
    uint32_t pdIndex = virtualPageNumber >> 10;

    if (pageDirectory[pdIndex] & 1) {
        uint32_t pageTableAddress = (pageDirectory[pdIndex] >> 12) << 12;
        uint32_t *pageTable = (uint32_t *) pageTableAddress;

        uint32_t ptIndex = virtualPageNumber & 0x3FF;
        // deallocate if present
        if (pageTable[ptIndex] & 1) {
            uint32_t physicalAddress = (pageTable[ptIndex] >> 12) << 12;
            PhysMem::dealloc_frame(physicalAddress);
            invlpg(virtualPageNumber << 12);
            pageTable[ptIndex] &= 0xFFFFFFFC;
        }
    }
}

void map_page(uint32_t virtualPageNumber, uint32_t physicalPageNumber, uint32_t *pageDirectory) {
    uint32_t pdIndex = virtualPageNumber >> 10;

    uint32_t pageTableAddress;
    uint32_t *pageTable;
    if (pageDirectory[pdIndex] & 1) {
        pageTableAddress = (pageDirectory[pdIndex] >> 12) << 12;
        pageTable = (uint32_t *) pageTableAddress;
    } else {
        pageTableAddress = PhysMem::alloc_frame();
        pageTable = (uint32_t *) pageTableAddress;
        pageDirectory[pdIndex] = (pageTableAddress >> 12) << 12;
        pageDirectory[pdIndex] |= 3; // make present r/w bits true
    }

    uint32_t ptIndex = virtualPageNumber & 0x3FF;
    if (!(pageTable[ptIndex] & 1)) {
        pageTable[ptIndex] = physicalPageNumber << 12;
        pageTable[ptIndex] |= 3; // make present and r/w bits true
    }
}

void global_init() {
    // store identity mapping - copy over to virtual thread process
    globalPD = (uint32_t *) PhysMem::alloc_frame();

    uint32_t numIdentityPages = kConfig.memSize / PhysMem::FRAME_SIZE;
    for (uint32_t vpn = 1; vpn < numIdentityPages; vpn++) {
        map_page(vpn, vpn, globalPD);
    }
}


// leave empty for now. We can use this later depending on implementation if needed
void per_core_init() {
    vmm_on((uint32_t) gheith::current()->pd);
    // MISSING();
}

void naive_munmap(void* p_) {
    uint32_t virtualAddress = (uint32_t) p_;

    // these are not unmappable. we always need these
    if (virtualAddress == kConfig.ioAPIC || virtualAddress == kConfig.localAPIC) {
        return;
    }

    auto currentTCB = gheith::current();
    currentTCB->vmmLinkedList->removeNode(virtualAddress);
}

void* naive_mmap(uint32_t sz_, Shared<Node> node, uint32_t offset_) {
    uint32_t pageBoundedSize = PhysMem::frameup(sz_);

    auto currentTCB = gheith::current();
    auto vmmNode = currentTCB->vmmLinkedList->addNode(pageBoundedSize, node, offset_);

    return (void *) vmmNode->virtualAddress;
}

} /* namespace vmm */

extern "C" void vmm_pageFault(uintptr_t va_, uintptr_t *saveState) {
    if (va_ >= 0 && va_ <= 0x1000) {
        Debug::panic("Segmentation fault");
    }
    auto currentTCB = gheith::current();

    if (va_ >= 0x80000000) {
        uint32_t allocatedAddress = PhysMem::alloc_frame();
        uint32_t virtualPageNumber = PhysMem::ppn(va_);
        uint32_t physicalPageNumber = PhysMem::ppn(allocatedAddress);
        VMM::map_page(virtualPageNumber, physicalPageNumber, currentTCB->pd);

        auto vmmNode = currentTCB->vmmLinkedList->getNode(va_);
        if (vmmNode == nullptr) {
            Debug::panic("NULL NODE");
        }

        if (vmmNode->inode != nullptr) {
            uint32_t pageBoundedAddress = PhysMem::framedown(va_);
            uint32_t startingAddress = vmmNode->fileOffset + (pageBoundedAddress - vmmNode->virtualAddress);
            uint32_t readSize = K::min(vmmNode->inode->size_in_bytes() - startingAddress, PhysMem::FRAME_SIZE);
            if (readSize > 0) {
                vmmNode->inode->read_all(startingAddress, readSize, (char *) allocatedAddress);
            }
        }
    }
}
