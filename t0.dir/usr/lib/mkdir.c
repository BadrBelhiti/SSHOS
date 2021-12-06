#include "libc.h"

int main(int argc, char** argv) {
    const char* name = argv[1];
    
    mkdir(name);

    return 0;
}