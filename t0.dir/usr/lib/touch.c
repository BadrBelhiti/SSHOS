#include "libc.h"

int touch(const char* fn);

int main(int argc, char** argv) {
    
    // ASSERT(argc == 1);
    printf("%d\n", argc);
    const char* name = argv[1];
    touch(name);

    return 0;
}