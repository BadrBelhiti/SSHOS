#include "libc.h"

int putchar(int c) {
    char t = (char)c;
    return write(1,&t,1);
}

int puts(const char* p) {
    char c;
    int count = 0;
    while ((c = *p++) != 0) {
        int n = putchar(c); 
        if (n < 0) return n;
        count ++;
    }
    putchar('\n');
    
    return count+1;
}

void cp(int from, int to) {
    while (1) {
        char buf[100];
        ssize_t n = read(from,buf,100);
        if (n == 0) break;
        if (n < 0) {
            printf("*** %s:%d read error, fd = %d\n",__FILE__,__LINE__,from);
            break;
        }
        char *ptr = buf;
        while (n > 0) {
            ssize_t m = write(to,ptr,n);
            if (m < 0) {
                printf("*** %s:%d write error, fd = %d\n",__FILE__,__LINE__,to);
                break;
            }
            n -= m;
            ptr += m;
        }
    }
}

int streq(const char* a, const char* b) {
    int i = 0;

    while (1) {
        char x = a[i];
        char y = b[i];
        if (x != y) return 0;
        if (x == 0) return 1;
        i++;
    }
}
