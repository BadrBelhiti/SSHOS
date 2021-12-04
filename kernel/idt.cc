#include "idt.h"
#include "stdint.h"
#include "debug.h"
#include "machine.h"
#include "smp.h"

#define PIC1 0x20
#define PIC2 0xA0
#define PIC1_COMMAND PIC1
#define PIC1_DATA (PIC1+1)
#define PIC2_COMMAND PIC2
#define PIC2_DATA (PIC2+1)
#define PIC_EOI 0x20

#define ICW1 0x11
#define ICW4 0x01

extern uint32_t idt[];
extern uint32_t kernelCS;

void IDT::init(void) {
    // // ICW1
    outb(PIC1_COMMAND, ICW1);
    outb(PIC2_COMMAND, ICW1);

    // ICW2, irq 0 to 7 is mapped to 0x20 to 0x27, irq 8 to F is mapped to 28 to 2F
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);

    // ICW3, connect master pic with slave pic
    outb(PIC1_DATA, 0x4);
    outb(PIC2_DATA, 0x2);

    // ICW4, set x86 mode
    outb(PIC1_DATA, 1);
    outb(PIC2_DATA, 1);

    // clear the mask register
    outb(PIC1_DATA, 0);
    outb(PIC2_DATA, 0);
}

void IDT::interrupt(int index, uint32_t handler) {
    int i0 = 2 * index;
    int i1 = i0 + 1;

    idt[i0] = (kernelCS << 16) + (handler & 0xffff);
    idt[i1] = (handler & 0xffff0000) | 0x8e00;
}

void IDT::trap(int index, uint32_t handler, uint32_t dpl) {
    int i0 = 2 * index;
    int i1 = i0 + 1;

    idt[i0] = (kernelCS << 16) + (handler & 0xffff);
    idt[i1] = (handler & 0xffff0000) | 0x8f00 | (dpl << 13);
}
