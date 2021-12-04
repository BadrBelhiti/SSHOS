#include "shell.h"
#include "ext2.h"
#include "ide.h"
#include "threads.h"
#include "network.h"

using namespace gheith;

uint32_t cpuReadIoApic(void *ioapicaddr, uint32_t reg) {
   uint32_t volatile *ioapic = (uint32_t volatile *)ioapicaddr;
   ioapic[0] = (reg & 0xff);
   return ioapic[4];
}

void kernelMain(void) {
    // Create shell
    Shell shell{false};

    // Declare current shell as debugging shell
    Debug::init(&shell);

    // Mount file system
    Shared<Ide> ide = Shared<Ide>::make(1);
    Shared<Ext2> fs = Shared<Ext2>::make(ide);

    TCB *me = current();
    me->fs = fs;
    me->shell = &shell;

    // Init network driver
    Network::init();

    // Print out IOAPIC to debug
    for (uint32_t i = 0; i < 100; i++) {
        Debug::printf("0x%x ", cpuReadIoApic((void*) kConfig.ioAPIC, i));
    }

    // Start shell
    shell.start();
}

