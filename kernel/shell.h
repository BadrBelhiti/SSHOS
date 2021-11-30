#ifndef SHELL_H_
#define SHELL_H_

#include "stdint.h"
#include "commands.h"

#define WHITE_ON_BLACK 0x0F
#define WHITE_ON_BLUE 0x1F
#define BUF_SIZE 4096

class BlockingLock;

struct ShellConfig {
    uint8_t theme = WHITE_ON_BLACK;
};

class Shell {
    private:
        ShellConfig config;
        CommandRunner *cmd_runner;
        BlockingLock *the_lock;
        char buffer[BUF_SIZE];
        uint32_t cursor = 0;
        uint32_t curr_cmd_start = 0;
        uint32_t leftShifts = 0;

    public:
        Shell(bool primitive);
        void start();
        void refresh();
        void vprintf(const char* fmt, va_list ap);
        void printf(const char* fmt, ...);
        void println(const char *str);
        void print_prefix();
        void clear();
        bool handle_backspace();
        bool handle_return();
        bool handle_del();
        bool handle_larrow();
        bool handle_rarrow();
        bool handle_normal(char key);
        void shift_charsLeft(uint32_t start);
        void shift_charsRight(uint32_t start);
        bool handle_key(char key);
        void set_theme(int theme);

    friend class CommandRunner;
};

#endif