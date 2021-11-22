#ifndef _SYS_H_
#define _SYS_H_

class SYS {
public:
    static void init(void);
};

int execl(const char* path, const char** args);

#endif
