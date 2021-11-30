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
    Debug::printf("iobase: %d\n", iobase);

    // Turn on network card
    outb(iobase + 0x52, 0x0);

    // Software reset
    outb(iobase + 0x37, 0x10);
    while ((inb(iobase + 0x37) & 0x10) != 0);

    // Initialize receive buffer
    rx_buffer = new char[RX_BUF_SIZE];
    outl(iobase + 0x30, (uintptr_t)rx_buffer);
}