#include "libc.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: rm <file name>\n");
        return -1;
    }

    char *fileName = argv[1];

    if (streq(fileName, ".") || streq(fileName, "..")) {
        printf("Bruh\n");
        return -1;
    }

    int fd = open(fileName);

    if (fd == -1) {
        printf("File %s doesn't exist\n", fileName);
        return -1;
    }

    printf("about to call remove structure\n");
    int result = removeStructure(fd, fileName[0] == '/');
    if (result < 0) {
        printf("You are not that guy pal\n");
    }
}