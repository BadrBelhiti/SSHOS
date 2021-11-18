#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#define STATUS_PORT 0x64
#define DATA_PORT 0x60
#define RETURN 0x0A
#define SPACE 0x20
#define BACKSPACE 0x08

/*
 * Reads keyboard input using Scan Code Set 1
 * (https://wiki.osdev.org/PS/2_Keyboard)
 */


char get_key();
namespace Keyboard {
    void init();
}

#endif