#include "keyboard.h"
#include "machine.h"

static char scancodes[256];

bool key_pressed() {
    return inb(STATUS_PORT) & 1;
}

char get_key() {
    while (!key_pressed());
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

    /* USEFUL KEYS */
    scancodes[0x9C] = RETURN;
    scancodes[0xB9] = SPACE;
    scancodes[0x8E] = BACKSPACE;
}