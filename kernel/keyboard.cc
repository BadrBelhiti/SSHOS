
#include "keyboard.h"
#include "machine.h"

static char scancodes[256];

// checks if a key is currently being pressed
bool key_pressed() {
    return inb(STATUS_PORT) & 1;
}

// returns version of inputted character when Shift is held down
char getShift(uint32_t notShift) {
    // Different key pressed from shift, while shift is still held down
    if (notShift > 0x89 || notShift < 0x8A) {
        char scancode = scancodes[inb(DATA_PORT)];
        if (scancode != RETURN && scancode != SPACE && scancode != BACKSPACE) {
            scancode = (char)((int)scancode - 32);
        }

        return scancode;
    }
    else {
        // get the SHIFT case characters for each one
        switch(notShift) {
            // numbers (0 - 9)
            case 0x82:
                return '!';
            case 0x83:
                return '@';
            case 0x84: 
                return '#';
            case 0x85:
                return '$';
            case 0x86:
                return '%';
            case 0x87:
                return '^';
            case 0x88:
                return '&';
            case 0x89:
                return '*';
            case 0x8A:
                return '(';
            case 0x8B:
                return ')';

            // other characters
            case 0x8C:
                return '_';
            case 0x8D:
                return '+';
            case 0xAB: 
                return '|';
            case 0x9A:
                return '{';
            case 0x9B: 
                return '}';
            case 0xA7:
                return ':';
            case 0xA8:
                return '\"';
            case 0xB3:
                return '<';
            case 0xB4:
                return '>';
            case 0xB5:
                return '?';
        }
    }

    // default case, if shift does not apply
    return notShift;
}

// gets the key being pressed (special case for Shift key)
char get_key() {
    while (!key_pressed());

    bool shiftPressed = false;
    uint32_t pressedKey = inb(DATA_PORT);
    if (pressedKey == 0xAA || pressedKey == 0xB6) {
        shiftPressed = true;     
        while (shiftPressed) {
            if (!key_pressed()) {
                // shift no longer held down
                shiftPressed = false;
                break;
            }

            uint32_t recentlyPressed = inb(DATA_PORT);
            if (recentlyPressed != 0xAA && recentlyPressed != 0xB6) {
                // new key pressed while shift is being held down
                return getShift(recentlyPressed);
            }
        }   
    }
    
    return scancodes[inb(DATA_PORT)];
}

void Keyboard::init() {
    /* LETTERS */
    scancodes[0x9E] = 'a';
    scancodes[0xB0] = 'b';
    scancodes[0xAE] = 'c';
    scancodes[0xA0] = 'd';
    scancodes[0x92] = 'e';
    scancodes[0xA1] = 'f';
    scancodes[0xA2] = 'g';
    scancodes[0xA3] = 'h';
    scancodes[0x97] = 'i';
    scancodes[0xA4] = 'j';
    scancodes[0xA5] = 'k';
    scancodes[0xA6] = 'l';
    scancodes[0xB2] = 'm';
    scancodes[0xB1] = 'n';
    scancodes[0x98] = 'o';
    scancodes[0x99] = 'p';
    scancodes[0x90] = 'q';
    scancodes[0x93] = 'r';
    scancodes[0x9F] = 's';
    scancodes[0x94] = 't';
    scancodes[0x96] = 'u';
    scancodes[0xAF] = 'v';
    scancodes[0x91] = 'w';
    scancodes[0xAD] = 'x';
    scancodes[0x95] = 'y';
    scancodes[0xAC] = 'z';

    /* NUMBERS */
    scancodes[0x82] = '1';
    scancodes[0x83] = '2';
    scancodes[0x84] = '3';
    scancodes[0x85] = '4';
    scancodes[0x86] = '5';
    scancodes[0x87] = '6';
    scancodes[0x88] = '7';
    scancodes[0x89] = '8';
    scancodes[0x8A] = '9';
    scancodes[0x8B] = '0';

    /* OTHER */
    scancodes[0x8C] = '-';
    scancodes[0x8D] = '=';
    scancodes[0xAB] = '\\';
    scancodes[0x9A] = '[';
    scancodes[0x9B] = ']';
    scancodes[0xA7] = ';';
    scancodes[0xA8] = '\'';
    scancodes[0xB3] = ',';
    scancodes[0xB4] = '.';
    scancodes[0xB5] = '/';

    /* USEFUL KEYS */
    scancodes[0x9C] = RETURN;
    scancodes[0xB9] = SPACE;
    scancodes[0x8E] = BACKSPACE;
    scancodes[0x8F] = TAB:
    scancodes[0x81] = ESCAPE;
}