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
    scancodes[0x9E] = 'A';
    scancodes[0xB0] = 'B';
    scancodes[0xAE] = 'C';
    scancodes[0xA0] = 'D';
    scancodes[0x92] = 'E';
    scancodes[0xA1] = 'F';
    scancodes[0xA2] = 'G';
    scancodes[0xA3] = 'H';
    scancodes[0x97] = 'I';
    scancodes[0xA4] = 'J';
    scancodes[0xA5] = 'K';
    scancodes[0xA6] = 'L';
    scancodes[0xB2] = 'M';
    scancodes[0xB1] = 'N';
    scancodes[0x98] = 'O';
    scancodes[0x99] = 'P';
    scancodes[0x90] = 'Q';
    scancodes[0x93] = 'R';
    scancodes[0x9F] = 'S';
    scancodes[0x94] = 'T';
    scancodes[0x96] = 'U';
    scancodes[0xAF] = 'V';
    scancodes[0x91] = 'W';
    scancodes[0xAD] = 'X';
    scancodes[0x95] = 'Y';
    scancodes[0xAC] = 'Z';

    /* USEFUL KEYS */
    scancodes[0x9C] = RETURN;
    scancodes[0xB9] = SPACE;
    scancodes[0x8E] = BACKSPACE;
}