#include "shell.h"
#include "video.h"
#include "keyboard.h"
#include "machine.h"
#include "atomic.h"

Shell::Shell() {}

void Shell::start() {
    memcpy(buffer, "root:/", 6);
    cursor = 6;
    curr_cmd_start = 6;
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
    // bool was = Interrupts::disable();
    clear_screen();

    uint32_t index = 0;
    uint32_t video_cursor = 0;
    while (index < 4096 && buffer[index] != 0) {
        if (buffer[index] == '\n') {
            video_cursor = move_offset_to_new_line(video_cursor);
        } else {
            set_char_at_video_memory(buffer[index], video_cursor);
            video_cursor += 2;
        }

        index++;
    }

    set_cursor(video_cursor);
    // Interrupts::restore(was);
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
    uint32_t cmd_end = cursor;

    // Echo command
    uint32_t cmd_size = cmd_end - curr_cmd_start;

    if (cmd_size != 0) {
        buffer[cursor++] = '\n';
    }

    memcpy(&buffer[cursor], &buffer[curr_cmd_start], cmd_size);

    cursor += cmd_size;

    memcpy(&buffer[cursor], "\nroot:/", 7);
    cursor += 7;
    curr_cmd_start = cursor;

    return true;
}

bool Shell::handle_normal(char key) {
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