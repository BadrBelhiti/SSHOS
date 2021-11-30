#ifndef _shared_h_
#define _shared_h_

template <typename T>
class Shared {

private:
    T* the_ptr;

    void construct(T *the_ptr) {
        this->the_ptr = the_ptr; // remember the pointer
        if (this->the_ptr != nullptr) {
            this->the_ptr->ref_count.add_fetch(1);
        }
    }

    void destruct() {
        if (this->the_ptr != nullptr) {
            if (this->the_ptr->ref_count.add_fetch(-1) == 0) {
                delete this->the_ptr;
            }
        }
    }

public:
    // Shared<Thing> x{new Thing()};
    explicit Shared(T* it) {
        construct(it);
    }

    // Shared<Thing> a{};
    Shared() {
        this->the_ptr = nullptr;
    }

    // we are creating a brand new shared object
    // points to an object on the heap that we have already created a smart pointer for
    // we need to make sure to add one to its reference count
    //
    // Shared<Thing> b { a };
    // Shared<Thing> c = b;
    // f(b);
    // return c;
    //
    Shared(const Shared& rhs) {
        construct(rhs.the_ptr);
    }

    ~Shared() {
        destruct();
    }

    // d->m();
    // dereferencing
    T* operator -> () const {
        return this->the_ptr;
    }

    // d = new Thing{};
    Shared<T>& operator=(T* rhs) {
        if (rhs != nullptr) {
            rhs->ref_count.add_fetch(1);
        }

        destruct();

        this->the_ptr = rhs;

        return *this;
    }

    // d = Thing{};
    Shared<T>& operator=(const Shared<T>& rhs) {
        if (rhs.the_ptr != nullptr) {
            rhs.the_ptr->ref_count.add_fetch(1);
        }

        destruct();

        this->the_ptr = rhs.the_ptr;

        return *this;
    }

    // a == b
    bool operator==(const Shared<T>& rhs) const {
        return this->the_ptr == rhs.the_ptr;
    }

    // a != b
    bool operator!=(const Shared<T>& rhs) const {
        return this->the_ptr != rhs.the_ptr;
    }

    // a == nullptr
    // a == new T{}
    bool operator==(T* rhs) {
        return this->the_ptr == rhs;
    }

    // a != nullptr
    // a != new T{}
    bool operator!=(T* rhs) {
        return this->the_ptr != rhs;
    }

    // e = Shared<Thing>::make(1,2,3);
    template <typename... Args>
    static Shared<T> make(Args... args) {
        return Shared<T>{new T(args...)};
    }

};

#endif
