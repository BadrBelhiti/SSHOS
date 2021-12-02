#include "libc.h"

int main(int argc, char** argv) {
    
    if (argc != 2) {
        printf("Usage: cat <file name>\n");
        return -1;
    }
    
    char *fileName = argv[1];
    int file_fd = open(fileName);
    printf("file fd: %d\n", file_fd);
    if (file_fd < 0) {
        printf("File %s doesn't exist\n", fileName);
        return -1;
    }

    cp(file_fd, 2);

    return 0;
}