#include "libc.h"

extern int println(char *str);

int main(int argc, char** argv) {
    
    println("In color now");

    shutdown();
    return 0;
}