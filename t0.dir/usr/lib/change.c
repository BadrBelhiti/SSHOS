#include "libc.h"

int opendir(const char* fn);
int readdir(int fd, char* buff_start, uint32_t max_size);

int main(int argc, char** argv) {
    
    int fd = opendir("/etc/testdir");
    // printf("file index: %d\n", fd);

    char *test_buff = (char *) 0x80008000;
    readdir(fd, test_buff, 1000);

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