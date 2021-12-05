#include "commands.h"
#include "libk.h"
#include "shell.h"
#include "sys.h"
#include "threads.h"
#include "vmm.h"

using namespace gheith;


CommandRunner::CommandRunner() {}

bool CommandRunner::execute(char *cmd) {
    TCB *me = current();

    // Get program name
    uint32_t program_name_size = 0;
    while (cmd[program_name_size] != 0 && cmd[program_name_size] != ' ') {
        program_name_size++;
    }

    char program_name[program_name_size + 1];
    program_name[program_name_size] = 0;
    memcpy(program_name, cmd, program_name_size);
    
    // Change format to /usr/bin/<program_name>
    char *full_program_name = new char[9 + program_name_size + 1];
    memcpy(full_program_name, "/usr/bin/", 9);
    memcpy(full_program_name + 9, program_name, program_name_size);
    full_program_name[9 + program_name_size] = 0;

    // Get program vnode
    Shared<Node> program_vnode = me->fs->find(me->fs->root, full_program_name);
    if (program_vnode == nullptr) {
        this->shell->printf("Program %s does not exist\n", full_program_name);
        return false;
    }

    // Get args
    uint32_t curr_index = program_name_size;
    uint32_t argc = 1;

    while (cmd[curr_index] != 0 && cmd[curr_index] != '>') {
        if (cmd[curr_index] == ' ' && cmd[curr_index + 1] != '>') {
            argc++;
        }
        curr_index++;
    }


    char **argv = new char*[argc + 1];

    // Reset for second loop
    uint32_t arg_begin = program_name_size + 1;
    uint32_t curr_arg = 0;
    curr_index = program_name_size + 1;
    char *first_arg = new char[program_name_size + 1];
    first_arg[program_name_size] = 0;
    memcpy(first_arg, program_name, program_name_size);
    argv[curr_arg++] = first_arg;


    bool redirection = false;
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
        if (cmd[curr_index] == '>') {
            redirection = true;
            break;
        }
        curr_index++;
    }

    // Get output file
    if (redirection) {
        curr_index += 2;
        uint32_t fn_len = 0;
        uint32_t file_start = curr_index;
        while (cmd[curr_index] != 0) {
            fn_len++;
            curr_index++;
        }

        if (fn_len == 0) {
            this->shell->printf("Cannot redirect to null file\n");
            return false;
        }

        // fn_len includes line terminator
        char *fn = new char[fn_len + 1];
        fn[fn_len] = 0;
        curr_index = file_start;
        while (cmd[curr_index] != 0) {
            fn[curr_index - file_start] = cmd[curr_index];
            curr_index++;
        }
        
        Shared<Node> output_vnode;
        // Get by absolute path
        if (fn[0] == '/') {
            output_vnode = find_by_absolute(fn);
        } else {
            output_vnode = me->fs->find(me->dir_inode, fn);
        }

        if (output_vnode == nullptr) {
            this->shell->printf("Redirection output file %s does not exist\n", fn);
            return false;
        }

        me->redirection_output = output_vnode;
    }


    // Args to execl are null terminated
    argv[argc] = 0;

    // User programs must run on different thread with same shell
    TCB *commandProcess = shellProgram(current(), make_pd(), 10, [me, full_program_name, argv] {
        execl(full_program_name, (const char**) argv);
    });


    uint32_t status = 1;
    wait(10, &status);

    current()->dir_inode = commandProcess->dir_inode;
    memcpy(current()->dir_name, commandProcess->dir_name, K::strlen(commandProcess->dir_name));
    current()->dir_name[K::strlen(commandProcess->dir_name)] = '\0';

    // Reset direction output for shell thread
    me->redirection_output = nullptr;
    // Debug::printf("in shell: %s\n", current()->dir_name);

    return true;
}