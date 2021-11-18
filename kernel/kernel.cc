#include "debug.h"
#include "ide.h"
#include "ext2.h"
#include "elf.h"
#include "machine.h"
#include "libk.h"
#include "config.h"
#include "threads.h"
#include "video.h"
#include "keyboard.h"

void kernelMain(void) {
    clear_screen();

    char line[256];
    uint32_t cursor = 0;

    while (true) {
        char key = get_key();

        // Check to see if key pressed is mapped
        if (key == 0) continue;

        // Check for enter
        if (key != RETURN) {
            if (key == BACKSPACE && cursor != 0) {
                cursor--;
                line[cursor] = 0;
                clear_screen();
                print_string(line);
            } else if (key != BACKSPACE) {
                line[cursor] = key;
                set_char_at_video_memory(key, 2 * cursor);
                set_cursor(2 * (cursor + 1));
                cursor++;
            }
        } else {
            // TODO: Implement return key logic
        }
    }
}

