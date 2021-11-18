#ifndef SHELL_H_
#define SHELL_H_

#include "stdint.h"

class Shell {
    private:
        char buffer[4096];
        uint32_t cursor = 0;
        uint32_t curr_cmd_start = 0;
    public:
        Shell();
        void start();
        void refresh();
        bool handle_backspace();
        bool handle_return();
        bool handle_normal(char key);
        bool handle_key(char key);
};

#endif