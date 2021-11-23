#include "shell.h"
#include "ext2.h"
#include "ide.h"
#include "threads.h"

using namespace gheith;

void kernelMain(void) {
    // Create shell
    Shell shell{};

    // Mount file system
    Shared<Ide> ide = Shared<Ide>::make(1);
    Shared<Ext2> fs = Shared<Ext2>::make(ide);

    TCB *me = current();
    me->fs = fs;
    me->shell = &shell;

    // Start shell
    shell.start();
}

