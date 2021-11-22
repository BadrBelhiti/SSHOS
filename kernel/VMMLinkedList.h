#include "ext2.h"
#include "physmem.h"
#include "vmm.h"

class VMMNode {
    public:
        uint32_t virtualAddress;
        uint32_t sizeInBytes;
        Shared<Node> inode;
        uint32_t fileOffset;
        VMMNode *next;
        uint32_t *pageDirectory;

        VMMNode(uint32_t virtualAddress, uint32_t sizeInBytes, Shared<Node> inode, uint32_t fileOffset, uint32_t *pageDirectory) {
            this->virtualAddress = virtualAddress;
            this->sizeInBytes = sizeInBytes;
            this->inode = inode;
            this->fileOffset = fileOffset;
            this->pageDirectory = pageDirectory;
            this->next = nullptr;
        }

        // implement destructor
        ~VMMNode() {
            uint32_t initVpn = PhysMem::ppn(virtualAddress);

            uint32_t numPages = sizeInBytes / PhysMem::FRAME_SIZE;
            for (uint32_t vpn = 0; vpn < numPages; vpn++) {
                VMM::delete_page(initVpn + vpn, pageDirectory);
            }
        }
    };

    class VMMLinkedList {
    public:
        VMMNode *first;
        uint32_t *pageDirectory;

        VMMLinkedList(uint32_t ioAPICAddress, uint32_t localAPICAddress, uint32_t *pageDirectory) {
            first = new VMMNode(0x7FFFFFFF, 1, Shared<Node>{}, 0, pageDirectory); // dummyNode
            first->next = new VMMNode(ioAPICAddress, PhysMem::FRAME_SIZE, Shared<Node>{}, 0, pageDirectory);
            first->next->next = new VMMNode(localAPICAddress, PhysMem::FRAME_SIZE, Shared<Node>{}, 0, pageDirectory);
            this->pageDirectory = pageDirectory;
        }

        ~VMMLinkedList() {
            // VMMNode *curNode = first;
            // while (curNode != nullptr) {
            //     VMMNode *toDelete = curNode;
            //     curNode = curNode->next;
            //     delete toDelete;
            // }
        }

        VMMNode *addNode(uint32_t sizeInBytes, Shared<Node> inode, uint32_t fileOffset) {
            VMMNode *curNode = first;
            while (curNode->next != nullptr && curNode->virtualAddress + 
                            curNode->sizeInBytes + sizeInBytes > curNode->next->virtualAddress) {
                curNode = curNode->next;
            }
            VMMNode *addedNode = new VMMNode(curNode->virtualAddress + curNode->sizeInBytes, 
                        sizeInBytes, inode, fileOffset, pageDirectory);
            addedNode->next = curNode->next;
            curNode->next = addedNode;
            
            return addedNode;
        }

        bool inRange(uint32_t lowerBound, uint32_t virtualAddress, uint32_t size) {
            return virtualAddress >= lowerBound && virtualAddress < lowerBound + size;
        }

        VMMNode *getNode(uint32_t virtualAddress) {
            VMMNode *curNode = first;
            while (curNode != nullptr && !inRange(curNode->virtualAddress, virtualAddress, curNode->sizeInBytes)) {
                curNode = curNode->next;
            }
            
            return curNode;
        }

        void removeNode(uint32_t virtualAddress) {
            VMMNode *oldNode;
            if (first->virtualAddress == virtualAddress) {
                oldNode = first;
                first = first->next;
                delete oldNode;
                return;
            }

            VMMNode *trailerNode = first;
            VMMNode *curNode = first->next;
            while (curNode != nullptr && curNode->virtualAddress != virtualAddress) {
                curNode = curNode->next;
                trailerNode = trailerNode->next;
            }

            oldNode = curNode;
            trailerNode->next = curNode->next;
            delete oldNode;
        }
    };