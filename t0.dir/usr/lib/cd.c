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

    if (name[0] == '/') {
        abs = (char*) name;

    } else {
        int len = getcwd(abs);
        int byte = len;
        int i = 0;
        while (name[byte-len] != '\0') {
            abs[byte] = name[i];
            i++;
            byte++;
        }
    }

    // pass in the absolute path
    chdir((const char*) abs);

    return 0;
}