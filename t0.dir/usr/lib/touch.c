#include "libc.h"

int main(int argc, char** argv) {
    const char* name = argv[1];
    
    touch(name);

    return 0;
}