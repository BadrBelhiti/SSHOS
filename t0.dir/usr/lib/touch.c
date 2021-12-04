#include "libc.h"

int touch(const char* fn);

int main(int argc, char** argv) {
    const char* name = argv[1];
    
    touch(name);

    return 0;
}