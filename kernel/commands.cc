#include "commands.h"
#include "libk.h"
#include "shell.h"

CommandRunner::CommandRunner() {

}

bool CommandRunner::execute(char *cmd) {
    // Get program name
    uint32_t program_name_size = 0;
    while (cmd[program_name_size] != 0 && cmd[program_name_size] != ' ') {
        program_name_size++;
    }

    char program_name[program_name_size + 1];
    program_name[program_name_size] = 0;
    memcpy(program_name, cmd, program_name_size);
    
    // Get args
    uint32_t curr_index = program_name_size;
    uint32_t argc = 1;

    while (cmd[curr_index] != 0) {
        if (cmd[curr_index] == ' ') {
            argc++;
        }
        curr_index++;
    }

    char **argv = new char*[argc];

    // Reset for second loop
    uint32_t arg_begin = program_name_size + 1;
    uint32_t curr_arg = 0;
    curr_index = program_name_size + 1;
    char *first_arg = new char[program_name_size + 1];
    first_arg[program_name_size] = 0;
    memcpy(first_arg, program_name, program_name_size);
    argv[curr_arg++] = first_arg;

    // Iterate through args
    while (true) {
        if (cmd[curr_index] == ' ' || cmd[curr_index] == 0) {
            char *arg = new char[curr_index - arg_begin + 1];
            arg[curr_index - arg_begin] = 0;
            memcpy(arg, &cmd[arg_begin], curr_index - arg_begin);
            argv[curr_arg++] = arg;
            arg_begin = curr_index + 1;
        }
        if (cmd[curr_index] == 0) break;
        curr_index++;
    }

    // Use args to execute command
    // TODO: This is only for testing. In reality, commands should be user programs.
    if (K::streq(program_name, "color")) {
        if (argc != 2) {
            shell->println((char*) "usage: color <DEFAULT|WINDOWS>");
            return false;
        } else {
            if (K::streq(argv[1], "default")) {
                shell->config.theme = 0x0F;
                shell->refresh();
                return true;
            }

            if (K::streq(argv[1], "windows")) {
                shell->config.theme = 0x1F;
                shell->refresh();
                return true;
            }

            shell->println((char*) "usage: color <DEFAULT|WINDOWS>");
        }
    }

    return false;
}