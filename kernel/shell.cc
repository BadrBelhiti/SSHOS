#include "shell.h"
#include "video.h"
#include "keyboard.h"
#include "machine.h"
#include "atomic.h"
#include "libk.h"
#include "blocking_lock.h"

Shell::Shell(bool primitive) {
    if (primitive) {
        return;
    }

    this->the_lock = new BlockingLock();
    this->cmd_runner = new CommandRunner();
    this->cmd_runner->shell = this;
}

void Shell::start() {
    print_prefix();
    refresh();

    while (true) {
        char key = get_key();

        // Check to see if key pressed is mapped
        if (key == 0) continue;

        // Handle key press
        bool refreshNeeded = handle_key(key);

        // Refresh screen if needed
        if (refreshNeeded) {
            refresh();
        }
    }
}

void Shell::refresh() {
    clear_screen(this->config);

    uint32_t index = 0;
    uint32_t video_cursor = 0;
    while (index < BUF_SIZE && buffer[index] != 0) {
        if (buffer[index] == '\n') {
            video_cursor = move_offset_to_new_line(video_cursor);
        } else {
            set_char_at_video_memory(buffer[index], video_cursor, this->config);
            video_cursor += 2;
        }

        index++;
    }

    set_cursor(video_cursor);
}

void Shell::println(const char *str) {
    LockGuardP g{the_lock};
    bool refreshNeeded = false;
    for (uint32_t i = 0; str[i] != 0; i++) {
        refreshNeeded |= handle_normal(str[i]);
    }

    refreshNeeded |= handle_normal('\n');

    if (refreshNeeded) {
        refresh();
    }
}

void Shell::print_prefix() {
    LockGuardP g{the_lock};
    memcpy(&buffer[cursor], "root:/", 6);
    cursor += 6;
    curr_cmd_start = cursor;
}

void Shell::clear() {
    clear_screen(this->config);
    print_prefix();
}

bool Shell::handle_backspace() {
    if (cursor != curr_cmd_start) {
        buffer[cursor - 1] = 0;
        cursor--;
        return true;
    }
    return false;
}

bool Shell::handle_return() {
    // Calculations needed before advancing cursor
    uint32_t cmd_end = cursor;

    uint32_t cmd_size = cmd_end - curr_cmd_start;

    // Append line break
    buffer[cursor++] = '\n';

    // Get current command
    char *cmd = new char[cmd_size + 1];
    cmd[cmd_size] = 0;

    memcpy(cmd, &buffer[curr_cmd_start], cmd_size);

    if (cmd_size != 0) {
        bool successful = cmd_runner->execute(cmd);
        if (!successful) {
            // println((char*) "Command failed");
        }
    }

    delete[] cmd;

    // TODO: Print prefix based on current working directory and current user
    print_prefix();

    return true;
}

bool Shell::handle_normal(char key) {
    if (cursor >= BUF_SIZE) {
        return false;
    }

    buffer[cursor] = key;
    cursor++;
    return true;
}

bool Shell::handle_key(char key) {
    switch (key) {
        case BACKSPACE:
            return handle_backspace();
        case RETURN:
            return handle_return();
        default:
            return handle_normal(key);
    }
}