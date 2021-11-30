#ifndef _list_h_
#define _list_h_

#include "atomic.h"
#include "debug.h"

// #define DEBUG

template <typename T>
class LinkedList {
    T * volatile first = nullptr;
    uint32_t size = 0;
public:
    LinkedList() : first(nullptr) {}
    LinkedList(const LinkedList&) = delete;

    void add_first(T *new_element) {
        ASSERT(new_element != nullptr);

        #ifdef DEBUG
            Debug::printf("Before adding 0x%x:\n", new_element);
            print();
        #endif

        if (first != nullptr) {
            new_element->next = first;
            first->prev = new_element;
        }

        first = new_element;
        size++;

        #ifdef DEBUG
            Debug::printf("After adding 0x%x:\n", new_element);
            print();
        #endif
    }

    void add_after(T *new_element, T *curr) {
        ASSERT(curr != nullptr);

        #ifdef DEBUG
            Debug::printf("Before adding 0x%x after 0x%x:\n", new_element, curr);
            print();
        #endif

        T *next = curr->next;

        curr->next = new_element;
        new_element->prev = curr;

        if (next != nullptr) {
            next->prev = new_element;
            new_element->next = next;
        }

        #ifdef DEBUG
            Debug::printf("After adding 0x%x after 0x%x:\n", new_element, curr);
            print();
        #endif

        size++;
    }

    void add_before(T *new_element, T *curr) {
        ASSERT(curr != nullptr);

        #ifdef DEBUG
            Debug::printf("Before adding 0x%x before 0x%x:\n", new_element, curr);
            print();
        #endif

        T *prev = curr->prev;

        curr->prev = new_element;
        new_element->next = curr;

        if (prev != nullptr) {
            prev->next = new_element;
            new_element->prev = prev;
        } else {
            first = new_element;
        }

        #ifdef DEBUG
            Debug::printf("After adding 0x%x before 0x%x:\n", new_element, curr);
            print();
        #endif

        size++;
    }

    void remove(T *to_remove) {
        ASSERT(to_remove != nullptr);

        #ifdef DEBUG
            Debug::printf("Before removing 0x%x:\n", to_remove);
            print();
        #endif

        T *next = to_remove->next;
        T *prev = to_remove->prev;

        // Case 0: Only element in list
        if (next == nullptr && prev == nullptr) {
            first = nullptr;
        }
        // Case 1: First element in list
        else if (prev == nullptr) {
            first = next;
            next->prev = nullptr;
        }
        // Case 2: Last element in list
        else if (next == nullptr) {
            prev->next = nullptr;
        }
        // Case 3: Middle of list
        else {
            prev->next = next;
            next->prev = prev;
        }

        #ifdef DEBUG
            Debug::printf("After removing 0x%x:\n", to_remove);
            print();
        #endif

        size--;
    }

    void print() {
        T *curr = first;
        Debug::printf("Linked list: ");
        while (curr != nullptr) {
            Debug::printf("0x%x", curr);
            if (curr->next != nullptr) {
                Debug::printf("->");
            }
            curr = curr->next;
        }
        Debug::printf("\n");
    }

    T *get_first() {
        return first;
    }

    uint32_t get_size() {
        return size;
    }
};

#endif