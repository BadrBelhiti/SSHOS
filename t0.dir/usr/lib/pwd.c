#include "libc.h"

int opendir(const char* fn);
int readdir(int fd, char* buff_start, uint32_t max_size);
int getcwd(char* buff);

int main(int argc, char** argv) {
    
    char *test_buff = (char *) 0x80008000;
    int len = getcwd(test_buff);
    test_buff[len] = '\0';
    printf("%s\n", test_buff);

    return 0;
}