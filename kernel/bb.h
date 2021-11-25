#ifndef _bb_h_
#define _bb_h_

#include "semaphore.h"
#include "atomic.h"

template <typename T>
class BoundedBuffer {
    uint32_t head;
    uint32_t n;
    Semaphore nEmpty;
    Semaphore nFull;
    Semaphore lock;
    const uint32_t N;
    T *data;
public:
    Atomic<uint32_t> ref_count;
    BoundedBuffer(uint32_t N) : head(0), n(0), nEmpty(N), nFull(0), lock(1), N(N), data(new T[N]), ref_count(0) {}
    ~BoundedBuffer() {
        delete []data;
    }

    BoundedBuffer(const BoundedBuffer&) = delete;

    void put(T t) {
        nEmpty.down();
        lock.down();
        data[(head + n) % N] = t;
        n++;
        lock.up();
        nFull.up();
    }

    T get() {
        T out;
        nFull.down();
        lock.down();
        out = data[head];
        head = (head + 1) % N;
        n--;
        lock.up();
        nEmpty.up();
        return out;
    }
        
};

#endif
