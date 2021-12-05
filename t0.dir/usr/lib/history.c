#include "libc.h"

int getcmd(char* buff, uint32_t line);

int main(int argc, char** argv) {
    
    char *buff = (char *) 0x80020000;

    int i = 0;
    int found = getcmd(buff, i);
    
    // go until no more commands found
    while (found >= 0) {
        if (i < 10) {
            printf("%c", ' ');
        }

        printf("%d", i);
        printf("%c", ' ');

        int byte = 0;
        while (byte < found) {
            printf("%c", buff[byte]);
            byte++;
        }
        printf("\n");
        i++;
        found = getcmd(buff, i);
    }

    return 0;
}