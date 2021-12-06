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

    // pass in the absolute path
    int fd = chdir((const char*) abs);

    if (fd == -1) {
        printf("Directory does not exist\n");
        return -1;
    }

    return 0;
}