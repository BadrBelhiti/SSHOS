#ifndef _PCI_H
#define _PCI_H

#include "stdint.h"
#include "machine.h"
#include "debug.h"

#define PCI_BASE_ADDRESS_0 0x10
#define PCI_BASE_ADDRESS_SPACE 0x01
#define PCI_BASE_ADDRESS_SPACE_MEMORY 0x00
#define PCI_BASE_ADDRESS_MEM_MASK (~0x0FUL )
#define PCI_IO_RESOURCE_MEM 0x00
#define PCI_BASE_ADDRESS_IO_MASK (~0x03UL )
#define PCI_IO_RESOURCE_IO 0x01
#define NETWORK_VENDOR_ID 0x10EC
#define PCI_CONFIG_DATA 0xCFC
#define PCI_INTERRUPT_LINE 0x3C

/* 
    NOTICE: This code is largely inspired by a working driver we found online.
    The original author's copyright and license notices are included below.
*/

//
// rtl8139.c
//
// RealTek RTL8129/RTL8139 PCI NIC network driver
//
// Written 1997-2002 by Donald Becker.
// Ported to sanos 2002 by Michael Ringgaard.
//
// This software may be used and distributed according to the terms of
// the GNU General Public License (GPL), incorporated herein by reference.
// Drivers based on or derived from this code fall under the GPL and must
// retain the authorship, copyright and license notice.  This file is not
// a complete program and may only be used when the entire operating
// system is licensed under the GPL.
// 
// This driver is for boards based on the RTL8129 and RTL8139 PCI ethernet
// chips.
// 
// The author may be reached as becker@scyld.com, or C/O
// Scyld Computing Corporation
// 410 Severn Ave., Suite 210
// Annapolis MD 21403
// 


typedef struct confadd {
    uint8_t reg : 8;
    uint8_t func : 3;
    uint8_t dev : 5;
    uint8_t bus : 8;
    uint8_t rsvd : 7;
    uint8_t enable : 1;
} confadd_t;

struct pci_cfg {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t interface;
    uint8_t sub_class;
    uint8_t base_class;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint8_t bus;
    uint8_t dev;
    uint8_t func;
    uint8_t irq;
    uint32_t base[6];
    uint32_t size[6];
    uint8_t type[6];
    uint32_t rom_base;
    uint32_t rom_size;
    uint16_t subsys_vendor;
    uint16_t subsys_device;
    uint8_t current_state;
};

uint32_t pci_read_config_dword(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg) {
    uint16_t base;

    union {
        confadd_t c;
        uint32_t n;
    } u;

    u.n = 0;
    u.c.enable = 1;
    u.c.rsvd = 0;
    u.c.bus = bus;
    u.c.dev = dev;
    u.c.func = func;
    u.c.reg = reg & 0xFC;

    outl(0xCF8, u.n);
    base = PCI_CONFIG_DATA + (reg & 0x03);
    return inl(base);
}

void pci_write_config_dword(uint32_t bus, uint32_t dev, uint32_t func, uint32_t reg, uint32_t val) {
    uint16_t base;

    union {
        confadd_t c;
        uint32_t n;
    } u;

    u.n = 0;
    u.c.enable = 1;
    u.c.rsvd = 0;
    u.c.bus = bus;
    u.c.dev = dev;
    u.c.func = func;
    u.c.reg = reg & 0xFC;

    outl(0xCF8, u.n);
    base = PCI_CONFIG_DATA + (reg & 0x03);
    outl(base, val);
}

uint32_t pci_size(uint32_t base, unsigned long mask) {
    uint32_t size = mask & base;
    size = size & ~(size-1);
    return(size-1);
}

void pci_read_irq(pci_cfg *cfg) {
    uint32_t dword = pci_read_config_dword(cfg->bus, cfg->dev, cfg->func, PCI_INTERRUPT_LINE);
    cfg->irq = dword & 0xFF;
}

void pci_read_bases(pci_cfg *cfg, uint32_t bases) {
    uint32_t l, sz, reg;
    uint32_t i;

    // Clear all previous bases and sizes informations
    bzero(cfg->base, sizeof(cfg->base));
    bzero(cfg->size, sizeof(cfg->size));
    bzero(cfg->type, sizeof(cfg->type));

    // Read informations about bases and sizes                      
    for (i = 0; i < bases; i++) {
        // Read bases and size                                  
        reg = PCI_BASE_ADDRESS_0 + (i << 2);
        l = pci_read_config_dword(cfg->bus, cfg->dev, cfg->func, reg);
        pci_write_config_dword(cfg->bus, cfg->dev, cfg->func, reg, ~0);
        sz = pci_read_config_dword(cfg->bus, cfg->dev, cfg->func, reg);
        pci_write_config_dword(cfg->bus, cfg->dev, cfg->func, reg, l);

        // Check if informations are valid                      
        if (!sz || sz == 0xFFFFFFFF)
            continue;
        if (l == 0xFFFFFFFF)
            l = 0;

        // Store informations                                   //
        if ((l & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_MEMORY) {
            cfg->base[i] = l & PCI_BASE_ADDRESS_MEM_MASK;
            cfg->size[i] = pci_size(sz, PCI_BASE_ADDRESS_MEM_MASK);
            cfg->type[i] = PCI_IO_RESOURCE_MEM;
        } else {
            cfg->base[i] = l & PCI_BASE_ADDRESS_IO_MASK;
            cfg->size[i] = pci_size(sz, PCI_BASE_ADDRESS_IO_MASK);
            cfg->type[i] = PCI_IO_RESOURCE_IO;
        }
    }
}

pci_cfg *find_network_card() {
    // Read 4 bytes at a time
    pci_cfg *cfg = new pci_cfg;
    uint32_t *tmp = (uint32_t *)cfg;

    // Scan through all buses, devices, and functions
    for (uint32_t bus = 0; bus < 4; bus++) {
        for (uint32_t dev = 0; dev < 32; dev++) {
            for (uint32_t func = 0; func < 8; func++) {

                for (uint32_t i = 0; i < 4; i++) {
                    tmp[i] = pci_read_config_dword(bus, dev, func, i << 2);
                }

                if (cfg->vendor_id == NETWORK_VENDOR_ID) {
                    cfg->bus = bus;
                    cfg->dev = dev;
                    cfg->func = func;
                    Debug::printf("Found ethernet card on bus: %d, dev: %d, func: %d, vendor_id: 0x%x\n", bus, dev, func, cfg->vendor_id);
                    pci_read_bases(cfg, 6);
                    return cfg;
                }
            }
        }
    }

    return nullptr;
}

#endif