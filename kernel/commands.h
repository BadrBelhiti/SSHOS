#ifndef COMMANDS_H_
#define COMMANDS_H_

#include "linked_list.h"

class Shell;

class Command {
    public:
        char *cmd;
};

class CommandRunner {
    private:
        Shell *shell;
        LinkedList<Command> *history;
    public:
        CommandRunner();
        bool execute(char *cmd);

    friend class Shell;
};

#endif