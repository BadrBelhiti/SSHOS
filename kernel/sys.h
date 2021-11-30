#ifndef _SYS_H_
#define _SYS_H_

#include "stdint.h"

class SYS {
public:
    static void init(void);
};

int execl(const char* path, const char** args);
int wait(int id, uint32_t *ptr);

#endif