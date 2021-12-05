#include "libc.h"

int opendir(const char* fn);
int readdir(int fd, char* buff_start, uint32_t max_size);
int chdir(const char* fn);
int getcwd(char* buff);

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Usage: cd <directory name>\n");
        return -1;
    }

    const char* name = argv[1];
    char* abs = (char *) 0x8000a000;

    abs = (char*) name;

    // if (name[0] == '/') {
    //     abs = (char*) name;
    //     printf("absolute: %s\n", abs);

    // } else {
    //     int len = getcwd(abs);

    //     printf("relative to %s\n", abs);

    //     abs[len] = '/';

    //     int byte = len+1;
    //     int i = 0;
    //     while (name[byte-len+1] != '\0') {
    //         abs[byte] = name[i];
    //         i++;
    //         byte++;
    //     }
    // }

    // pass in the absolute path
    // printf("passing in %s\n", abs);
    int fd = chdir((const char*) abs);

    if (fd == -1) {
        printf("Directory does not exist\n");
        return -1;
    }

    return 0;
}