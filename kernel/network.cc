#include "network.h"
#include "machine.h"
#include "pci.h"

uint32_t get_iobase() {
    pci_cfg *network_card = find_network_card();

    if (network_card == nullptr) {
        Debug::printf("Could not find PCI network card\n");
        return -1;
    }

    for (uint32_t i = 0; i < 6; i++) {
        if (network_card->base[i] != 0) {
            return network_card->base[i];
        }
    }

    return -1;
}

Network::Network() {
    // Get iobase for network card
    iobase = get_iobase();
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
    outb(iobase + 2 * 0x3E, 0x01); // bits 7..0
    outb(iobase + 2 * 0x3E + 1, 0x00); // bits 15..8

    uint32_t *dwords = (uint32_t*) rx_buffer;
    while (dwords[0] == 0);
    Debug::printf("0x%x\n", dwords[0]);
}