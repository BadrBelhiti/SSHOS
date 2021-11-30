#ifndef _barrier_h_
#define _barrier_h_

#include "atomic.h"
#include "semaphore.h"
#include "condition.h"
#include "debug.h"
#include "shared.h"

class Barrier {
    Atomic<int32_t> count;
    Semaphore sem;
    Atomic<uint32_t> ref_count;
public:

    Barrier(uint32_t count) : count(count),sem(0), ref_count(0) {}

    Barrier(const Barrier&) = delete;
    
    void sync() {
        int x = count.add_fetch(-1);
        if (x < 0) Debug::panic("count went negative in barrier");
        if (x == 0) {
            sem.up();
        } else {
            sem.down();
            sem.up();
        }
    }

    friend class Shared<Barrier>;
};

#endif

