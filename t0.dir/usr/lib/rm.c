#include "libc.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Usage: rm <file name>\n");
        return -1;
    }

    char *fileName = argv[1];

    if (streq(fileName, ".") || streq(fileName, "..")) {
        printf("Can't delete . or ..");
        return -1;
    }

    int result = removeStructure(fileName);
    if (result < 0) {
        printf("Unable to delete file\n");
    }
}