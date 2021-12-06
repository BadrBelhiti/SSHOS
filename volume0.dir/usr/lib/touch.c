#include "libc.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: touch <file name>\n");
    }
    const char* name = argv[1];
    
    touch(name);

    return 0;
}