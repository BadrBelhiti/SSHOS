#include "debug.h"
#include "smp.h"
#include "debug.h"
#include "config.h"
#include "machine.h"
#include "ext2.h"
#include "shared.h"
#include "threads.h"
#include "vmm.h"
#include "user_files.h"
#include "semaphore.h"
#include "future.h"


namespace gheith {
    Atomic<uint32_t> TCB::next_id{0};

    TCB** activeThreads;
    TCB** idleThreads;

    Queue<TCB,InterruptSafeLock> readyQ{};
    Queue<TCB,InterruptSafeLock> zombies{};

    TCB* current() {
        auto was = Interrupts::disable();
        TCB* out = activeThreads[SMP::me()];
        Interrupts::restore(was);
        return out;
    }

    void entry() {
        auto me = current();
        vmm_on((uint32_t)me->pd);
        sti();
        me->doYourThing();
        stop();
    }

    void delete_zombies() {
        while (true) {
            auto it = zombies.remove();
            if (it == nullptr) return;
            // delete it;
        }
    }

    void schedule(TCB* tcb) {
        if (!tcb->isIdle) {
            readyQ.add(tcb);
        }
    }

    struct IdleTcb: public TCB {
        IdleTcb(): TCB(true) {}
        void doYourThing() override {
            Debug::panic("should not call this");
        }
        uint32_t interruptEsp() override {
            // idle threads never enter user mode, this should be ok
            return 0;
        }
    };

    TCB::TCB(bool isIdle) : isIdle(isIdle), id(next_id.fetch_add(1)), ref_count(0) {
        saveArea.tcb = this;
        pd = make_pd();

        saveArea.cr3 = (uint32_t) pd;

        // Initialize resources
        open_files = new Shared<OpenFile>[10]();
        semaphores = new Shared<Semaphore>[10]();
        children = new TCB*[10]();

        // Initialize open files with stdio
        open_files[0] = Shared<OpenFile>::make(0, false, false, true);
        open_files[1] = Shared<OpenFile>::make(1, false, true, true);
        open_files[2] = Shared<OpenFile>::make(2, false, true, true);

        for (uint32_t i = 3; i < 10; i++) {
            open_files[i] = nullptr;
        }

        // Zero-initialize semaphores
        for (uint32_t i = 0; i < 10; i++) {
            semaphores[i] = nullptr;
        }

        // Zero-initialize children
        for (uint32_t i = 0; i < 10; i++) {
            children[i] = nullptr;
        }

        // Initialize waiting mechanism
        exit = new Future<uint32_t>();
        ASSERT(exit != nullptr);

        // set the current directory to root
        // curr_dir->dir_inode = fs->root;
        // // curr_dir->dir_name[0] = '/';
        // // curr_dir->dir_name[1] = '\0';
        // dir_inode = new Shared<Node>();
        dir_name = new char[50];
    }

    TCB::~TCB() {
        delete_pd(pd);
    }
};

void threadsInit() {
    using namespace gheith;
    activeThreads = new TCB*[kConfig.totalProcs]();
    idleThreads = new TCB*[kConfig.totalProcs]();

    // swiched to using idle threads in order to discuss in class
    for (unsigned i=0; i<kConfig.totalProcs; i++) {
        idleThreads[i] = new IdleTcb();
        activeThreads[i] = idleThreads[i];
    }

    // The reaper
    thread([] {
        //Debug::printf("| starting reaper\n");
        while (true) {
            ASSERT(!Interrupts::isDisabled());
            delete_zombies();
            yield();
        }
    });
    
}

void yield() {
    using namespace gheith;
    block(BlockOption::CanReturn,[](TCB* me) {
        schedule(me);
    });
}

void stop() {
    using namespace gheith;

    while(true) {
        block(BlockOption::MustBlock,[](TCB* me) {
            if (!me->isIdle) {
                zombies.add(me);
            }
        });
        ASSERT(current()->isIdle);
    }
}
