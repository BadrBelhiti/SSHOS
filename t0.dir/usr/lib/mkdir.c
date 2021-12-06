#include "libc.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: mkdir <directory name>\n");
        return -1;
    }

    const char* name = argv[1];
    
    mkdir(name);

    return 0;
}