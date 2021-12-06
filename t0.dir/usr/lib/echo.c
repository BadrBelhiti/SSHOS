#include "libc.h"

int main(int argc, char** argv) {
    
    uint32_t len = 0;

    for (uint32_t i = 1; i < argc; i++) {
        const char *curr_arg = argv[i];
        for (uint32_t j = 0; curr_arg[j] != 0; j++) {
            len++;
        }

        // For space
        len++;
    }

    // For line break
    len++;
    
    char buffer[len + 1];
    buffer[len] = 0;

    uint32_t curr_index = 0;

    for (uint32_t i = 1; i < argc; i++) {
        const char *curr_arg = argv[i];
        for (uint32_t j = 0; curr_arg[j] != 0; j++) {
            buffer[curr_index++] = curr_arg[j];
        }

        // For space
        buffer[curr_index++] = ' ';
    }

    buffer[curr_index] = '\n';
    write(2, buffer, len);

    return 0;
}