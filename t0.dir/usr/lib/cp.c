#include "libc.h"

int getcwd(char* buff);
int copy(char* from, char* to);

int main(int argc, char** argv) {

    if (argc != 3) {
        printf("Usage: cp <src file> <dest file>\n");
        return -1;
    }

    char *from = argv[1];
    char *to = argv[2];

    int res = copy(from, to);

    if (res < 0) {
        printf("Copy failed");
    }
    
    printf("\n");

    return 0;
}