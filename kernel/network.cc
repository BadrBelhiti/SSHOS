#include "network.h"
#include "machine.h"
#include "pci.h"
#include "idt.h"

uint32_t get_iobase(pci_cfg *cfg) {

    for (uint32_t i = 0; i < 6; i++) {
        if (cfg->base[i] != 0) {
            return cfg->base[i];
        }
    }

    return -1;
}

Network::Network() {
    // Find network card on PCI
    pci_cfg *network_card = find_network_card();

    if (network_card == nullptr) {
        Debug::printf("Could not find PCI network card\n");
        return;
    }

    // Get iobase for network card
    iobase = get_iobase(network_card);
    Debug::printf("iobase: 0x%x\n", iobase);

    // Turn on network card
    outb(iobase + 0x52, 0x0);

    // Software reset
    outb(iobase + 0x37, 0x10);
    while ((inb(iobase + 0x37) & 0x10) != 0);

    // Initialize receive buffer
    rx_buffer = new char[RX_BUF_SIZE];
    bzero(rx_buffer, RX_BUF_SIZE);
    outl(iobase + 0x30, (uintptr_t)rx_buffer);

    // Set IMR + ISR
    outb(iobase + 2 * 0x3C, 0x05); // bits 7..0
    outb(iobase + 2 * 0x3C + 1, 0x00); // bits 15..8

    // Configure receiving buffer
    outl(iobase + 0x44, 0xf | (1 << 7));

    // Enable receiving and transmitting
    outb(iobase + 0x37, 0x0C);

    // ISR Handler
    // outb(iobase + 2 * 0x3E, 0x01); // bits 7..0
    // outb(iobase + 2 * 0x3E + 1, 0x00); // bits 15..8

    listen();

    // Register interrupt handler
    // pci_read_irq(network_card);
    // Debug::printf("irq: 0x%x\n", network_card->irq);
    // IDT::interrupt(32 + network_card->irq, (uint32_t) network_int);
}

void Network::get_packet() {
    // Wait for packet
    while ((inb(iobase + CHIP_CMD) & RX_BUF_EMPTY) != 0);

    Debug::printf("Packet: 0x%x\n", *((uint32_t*)rx_buffer + 1));
    while(true);

    // Advance rx buffer
    Debug::printf("rx_buffer: 0x%x\n", (uint32_t) rx_buffer);
    rx_buffer += 1024;
    outl(iobase + 0x30, (uintptr_t)rx_buffer);
    outb(iobase + 0x37, 0x0C);
}

void Network::listen() {
    while (true) {
        get_packet();
        Debug::printf("Received packet\n");
    }
}

void Network::network_int(uint32_t *reg) {
    Debug::printf("Interrupt triggered\n");

    // Acknowledge packet
    outb(iobase + 2 * 0x3E, 0x05); // bits 7..0
    outb(iobase + 2 * 0x3E + 1, 0x00); // bits 15..8
}