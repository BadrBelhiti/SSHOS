#include "libc.h"

int main(int argc, char** argv) {
    
    int fd = opendir("/");
    // printf("file index: %d\n", fd);

    char *test_buff = (char *) 0x80008000;
    readdir(fd, test_buff, len(fd));

    unsigned int byte = 0;

    while (test_buff[byte] != '\0') {
        while (test_buff[byte] != '\0') {
            printf("%c", test_buff[byte]);
            byte++;
        }
        printf("\t");
        byte++;
    }

    printf("\n");

    return 0;
}