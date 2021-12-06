#include "libc.h"

int getcwd(char* buff);

int main(int argc, char** argv) {
    
    char *buff = (char *) 0x8000c000;
    getcwd(buff);
    int fd = opendir(buff);
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