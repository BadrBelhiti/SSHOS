#include "debug.h"
#include "ide.h"
#include "ext2.h"
#include "elf.h"
#include "machine.h"
#include "libk.h"
#include "config.h"
#include "threads.h"

void kernelMain(void) {
    // Initialize file system and load program file
    auto d = Shared<Ide>::make(1);
    auto fs = Shared<Ext2>::make(d);
    gheith::current()->fs = fs;
    auto root = fs->root;
    auto sbin = fs->find(root, "sbin");
    auto init = fs->find(sbin, "init");

    // Load program ELF
    uint32_t e = ELF::load(init);

    // Set up user stack with args
    auto userEsp = K::min(kConfig.ioAPIC,kConfig.localAPIC);
    uint32_t *userStack = (uint32_t*) userEsp;
    char *programName = (char*) 0xFFFFF000;
    uint32_t *pointerToArg = (uint32_t*) 0xFFFFFF00;
    *pointerToArg = (uint32_t) programName;
    memcpy(programName, "init", 5);
    userStack[-2] = 1;
    userStack[-1] = (uint32_t) pointerToArg;
    userEsp -= 8;

    // Debug::printf("*** user esp %x\n",userEsp);
    // Current state:
    //     - %eip points somewhere in the middle of kernelMain
    //     - %cs contains kernelCS (CPL = 0)
    //     - %esp points in the middle of the thread's kernel stack
    //     - %ss contains kernelSS
    //     - %eflags has IF=1
    // Desired state:
    //     - %eip points at e
    //     - %cs contains userCS (CPL = 3)
    //     - %eflags continues to have IF=1
    //     - %esp points to the bottom of the user stack
    //     - %ss contain userSS
    // User mode will never "return" from switchToUser. It will
    // enter the kernel through interrupts, exceptions, and system
    // calls.

    // Jump to user program
    switchToUser(e,userEsp,0);
    Debug::panic("*** implement switchToUser in machine.S\n");
}

