#include "libc.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: rm <file name>\n");
        return -1;
    }
    
    char *fileName = argv[1];
    int fd = open(fileName);
    if (fd < 0) {
        printf("File %s doesn't exist\n", fileName);
        return -1;
    }

    return removeStructure(fd, fileName[0] == '/');
}