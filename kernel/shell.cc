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
        } 
        else if (buffer[index] == '\t'){
            for (int i = 0; i < 4; i++) {
                set_char_at_video_memory(32, video_cursor, this->config);
                video_cursor += 2;
            }
        }    
        else {
            set_char_at_video_memory(buffer[index], video_cursor, this->config);
            video_cursor += 2;
        }

        index++;
    }

    set_cursor(video_cursor - 2*leftShifts);
}

void Shell::vprintf(const char* fmt, va_list ap) {
    the_lock->lock();
    K::vsnprintf(*this, 1000, fmt, ap);
    the_lock->unlock();
}

void Shell::printf(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt,ap);
    va_end(ap);
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
        shift_charsLeft(cursor);
        return true;
    }
    return false;
}

bool Shell::handle_return() {
    // Calculations needed before advancing cursor
    uint32_t cmd_end = cursor;

    uint32_t cmd_size = cmd_end - curr_cmd_start;

    // Go to end of line in the case cursor has been moved
    while (buffer[cursor] != 0) {
        cursor++;
    }
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
        
        commands[command_count] = cmd;
        commandSizes[command_count] = cmd_size;
        command_count++;
    }
    else {
        delete[] cmd;
    }

    // TODO: Print prefix based on current working directory and current user
    print_prefix();
    leftShifts = 0;
    currCommand = command_count;
    return true;
}

bool Shell::handle_del() {
    // delete the character the cursor is currently at
    if (buffer[cursor] == 0) {
        return false;
    }

    shift_charsLeft(cursor);
    leftShifts--;
    return true;
}

bool Shell::handle_larrow() {
    // move cursor left
    if (cursor <= curr_cmd_start) {
        return false;
    }
    
    cursor--;
    leftShifts++;
    return true;
}

bool Shell::handle_rarrow() {
    // move cursor right
    if (cursor >= 4096 || buffer[cursor] == 0) {
        return false;
    }

    cursor++;
    leftShifts--;
    return true;
}

bool Shell::handle_uarrow() {
    // up arrow
    if (currCommand == 0) {
        return false;
    }
    else if (currCommand == command_count) {
        // temporarily store the current command being typed
        uint32_t cmd_size = cursor - curr_cmd_start;
        char *cmd = new char[cmd_size + 1];
        cmd[cmd_size] = 0;

        memcpy(cmd, &buffer[curr_cmd_start], cmd_size);
        commands[currCommand] = cmd;
        commandSizes[currCommand] = cmd_size;
    }

    // retrieve past command, copy over the command and zero out extra characters in buffer
    currCommand--;
    memcpy(&buffer[curr_cmd_start], commands[currCommand], commandSizes[currCommand]);
    for (uint32_t i = curr_cmd_start + commandSizes[currCommand]; i <= cursor; i++) {
        buffer[i] = 0;
    }

    leftShifts = 0;
    cursor = curr_cmd_start + commandSizes[currCommand];
    return true;
}

bool Shell::handle_darrow() {
    // down arrow
    if (currCommand == command_count) {
        return false;
    }

    // retrieve next command, copy it over and zero out extra characters in buffer
    currCommand++;
    memcpy(&buffer[curr_cmd_start], commands[currCommand], commandSizes[currCommand]);
    for (uint32_t i = curr_cmd_start + commandSizes[currCommand]; i <= cursor; i++) {
        buffer[i] = 0;
    }

    leftShifts = 0;
    cursor = curr_cmd_start + commandSizes[currCommand];
    return true;
}

bool Shell::handle_normal(char key) {
    if (cursor >= BUF_SIZE) {
        return false;
    }

    if (buffer[cursor] != 0) {
        shift_charsRight(cursor);
    }
    buffer[cursor] = key;
    cursor++;
    return true;
}

void Shell::shift_charsRight(uint32_t start) {
    for (uint32_t i = 4095; i > start; i--) {
        buffer[i] = buffer[i - 1];
    }   
}

void Shell::shift_charsLeft(uint32_t start) {
    for (uint32_t i = start; i < 4095; i++) {
        buffer[i] = buffer[i + 1];
    }   
}

bool Shell::handle_key(char key) {
    switch (key) {
        case BACKSPACE:
            return handle_backspace();
        case RETURN:
            return handle_return();
        case DEL:
            return handle_del();
        case LARROW:
            return handle_larrow();
        case RARROW:
            return handle_rarrow();
        case UARROW:
            return handle_uarrow();
        case DARROW:
            return handle_darrow();
        default:
            return handle_normal(key);
    }
}

void Shell::set_theme(int theme) {
    this->config.theme = theme;
}