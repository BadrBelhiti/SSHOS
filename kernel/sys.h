#ifndef _SYS_H_
#define _SYS_H_

#include "stdint.h"
#include "ext2.h"

class SYS {
public:
    static void init(void);
};

Shared<Node> find_by_absolute(const char* name);
int execl(const char* path, const char** args);
int wait(int id, uint32_t *ptr);

#endif
