#ifndef SHELL_H_
#define SHELL_H_

#include "stdint.h"
#include "commands.h"

#define WHITE_ON_BLACK 0x0F

struct ShellConfig {
    uint8_t theme = WHITE_ON_BLACK;
};

class Shell {
    private:
        ShellConfig config;
        CommandRunner *cmd_runner;
        char buffer[4096];
        uint32_t cursor = 0;
        uint32_t curr_cmd_start = 0;
    public:
        Shell();
        void start();
        void refresh();
        void println(char *str);
        bool handle_backspace();
        bool handle_return();
        bool handle_tab();
        bool handle_normal(char key);
        bool handle_key(char key);

    friend class CommandRunner;
};

#endif