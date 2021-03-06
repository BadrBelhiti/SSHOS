#include "libk.h"
#include "debug.h"

long K::strlen(const char* str) {
    long n = 0;
    while (*str++ != 0) n++;
    return n;
}

int K::isdigit(int c) {
    return (c >= '0') && (c <= '9');
}

bool K::streq(const char* a, const char* b) {
    int i = 0;

    while (true) {
        char x = a[i];
        char y = b[i];
        if (x != y) return false;
        if (x == 0) return true;
        i++;
    }
}

unsigned int K::strcpy(char *dest, char *src) {
    uint32_t copied = 0;
    for (uint32_t i = 0; src[i] != 0; i++) {
        dest[i] = src[i];
        copied++;
    }

    dest[copied] = 0;
    return copied;
}


extern "C" void __cxa_pure_virtual() {
    Debug::panic("__cxa_pure_virtual called\n");
}
