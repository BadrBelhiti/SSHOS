#include "libc.h"

int main(int argc, char** argv) {
    
    if (argc != 2) {
        printf("Usage: atto <file name>\n");
        return -1;
    }
    
    char *fileName = argv[1];
    int file_fd = open(fileName);
    if (file_fd < 0) {
        printf("File %s doesn't exist\n", fileName);
        return -1;
    }

    char buffer[4096];
    uint32_t bytesRead = 0;
    while (1) {
        uint32_t readCount = readShellLine(buffer + bytesRead);
        buffer[bytesRead + readCount] = 0;
        if (streq(buffer + bytesRead, ":wq")) {
            write(file_fd, buffer, bytesRead);
            // write to file and quit
            break;
        } else if (streq(buffer + bytesRead, ":q")) {
            // quit
            break;
        }
        printf("\nbuffer: %s\n", buffer + bytesRead);
        bytesRead += readCount;
        buffer[bytesRead++] = '\n';
    }
    printf("\n");

    return 0;
}