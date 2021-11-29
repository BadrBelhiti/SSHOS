
#include "keyboard.h"
#include "machine.h"

static char scancodes[256];
bool shiftPressed = false;
bool capsReleased = true;
bool capsLock = false;

// checks if a key is currently being pressed
bool key_pressed() {
    return inb(STATUS_PORT) & 1;
}

// returns version of inputted character when Shift is held down
char getShift(uint32_t notShift) {
    char scancode = scancodes[notShift];
    // Different key pressed from shift, while shift is still held down
    if (scancode == RETURN || scancode == SPACE || scancode == BACKSPACE || scancode == TAB) {
        return scancode;
    }
    else if (scancode >= 'a' && scancode <= 'z') {
        scancode = (char)((int)scancode - 32);
        return scancode;
    }
    else {
        // get the SHIFT case characters for each one
        switch(notShift) {
            // numbers (0 - 9)
            case 0x02:
                return '!';
            case 0x03:
                return '@';
            case 0x04: 
                return '#';
            case 0x05:
                return '$';
            case 0x06:
                return '%';
            case 0x07:
                return '^';
            case 0x08:
                return '&';
            case 0x09:
                return '*';
            case 0x0A:
                return '(';
            case 0x0B:
                return ')';

            // other characters
            case 0x0C:
                return '_';
            case 0x0D:
                return '+';
            case 0x2B: 
                return '|';
            case 0x1A:
                return '{';
            case 0x1B: 
                return '}';
            case 0x27:
                return ':';
            case 0x28:
                return '\"';
            case 0x33:
                return '<';
            case 0x34:
                return '>';
            case 0x35:
                return '?';
        }
    }

    // default case, if shift does not apply
    return 0;
}

// gets the key being pressed (special case for Shift key)
char get_key() {
    while (!key_pressed());

    uint32_t pressedKey = inb(DATA_PORT);

    // handle shifts
    if (pressedKey == 0x2A || pressedKey == 0x36) {
        shiftPressed = true;     

    }
    else if (pressedKey == 0xAA || pressedKey == 0xB6) {
        shiftPressed = false;
    }
    // handle caps lock
    else if (pressedKey == 0x3A) {
        if (capsReleased) {
            if (capsLock) {
                capsLock = false;
            }
            else {
                capsLock = true;
            }
            capsReleased = false;
        }
    } 
    else if (pressedKey == 0xBA) {
        capsReleased = true;
    }

    if (shiftPressed ^ capsLock) {
        return getShift(pressedKey);
    }
    return scancodes[pressedKey];
}

void Keyboard::init() {
    /* LETTERS */
    scancodes[0x1E] = 'a';
    scancodes[0x30] = 'b';
    scancodes[0x2E] = 'c';
    scancodes[0x20] = 'd';
    scancodes[0x12] = 'e';
    scancodes[0x21] = 'f';
    scancodes[0x22] = 'g';
    scancodes[0x23] = 'h';
    scancodes[0x17] = 'i';
    scancodes[0x24] = 'j';
    scancodes[0x25] = 'k';
    scancodes[0x26] = 'l';
    scancodes[0x32] = 'm';
    scancodes[0x31] = 'n';
    scancodes[0x18] = 'o';
    scancodes[0x19] = 'p';
    scancodes[0x10] = 'q';
    scancodes[0x13] = 'r';
    scancodes[0x1F] = 's';
    scancodes[0x14] = 't';
    scancodes[0x16] = 'u';
    scancodes[0x2F] = 'v';
    scancodes[0x11] = 'w';
    scancodes[0x2D] = 'x';
    scancodes[0x15] = 'y';
    scancodes[0x2C] = 'z';

    /* NUMBERS */
    scancodes[0x02] = '1';
    scancodes[0x03] = '2';
    scancodes[0x04] = '3';
    scancodes[0x05] = '4';
    scancodes[0x06] = '5';
    scancodes[0x07] = '6';
    scancodes[0x08] = '7';
    scancodes[0x09] = '8';
    scancodes[0x0A] = '9';
    scancodes[0x0B] = '0';

    /* OTHER */
    scancodes[0x0C] = '-';
    scancodes[0x0D] = '=';
    scancodes[0x2B] = '\\';
    scancodes[0x1A] = '[';
    scancodes[0x1B] = ']';
    scancodes[0x27] = ';';
    scancodes[0x28] = '\'';
    scancodes[0x33] = ',';
    scancodes[0x34] = '.';
    scancodes[0x35] = '/';

    /* USEFUL KEYS */
    scancodes[0x9C] = RETURN;
    scancodes[0x39] = SPACE;
    scancodes[0x0E] = BACKSPACE;
    scancodes[0x0F] = TAB;
    // scancodes[0x81] = ESCAPE;
}