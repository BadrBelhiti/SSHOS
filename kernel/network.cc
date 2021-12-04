#include "network.h"
#include "machine.h"
#include "pci.h"
#include "idt.h"
#include "config.h"

using namespace Network;

static char *rx_buffer;
static uint32_t iobase;

/* Symbolic offsets to registers. */
enum RTL8139_registers {
	MAC0=0,			/* Ethernet hardware address. */
	MAR0=8,			/* Multicast filter. */
	TxStatus0=0x10,		/* Transmit status (four 32bit registers). */
	TxAddr0=0x20,		/* Tx descriptors (also four 32bit). */
	RxBuf=0x30, RxEarlyCnt=0x34, RxEarlyStatus=0x36,
	ChipCmd=0x37, RxBufPtr=0x38, RxBufAddr=0x3A,
	IntrMask=0x3C, IntrStatus=0x3E,
	TxConfig=0x40, RxConfig=0x44,
	Timer=0x48,		/* general-purpose counter. */
	RxMissed=0x4C,		/* 24 bits valid, write clears. */
	Cfg9346=0x50, Config0=0x51, Config1=0x52,
	TimerIntrReg=0x54,	/* intr if gp counter reaches this value */
	MediaStatus=0x58,
	Config3=0x59,
	MultiIntr=0x5C,
	RevisionID=0x5E,	/* revision of the RTL8139 chip */
	TxSummary=0x60,
	MII_BMCR=0x62, MII_BMSR=0x64, NWayAdvert=0x66, NWayLPAR=0x68,
	NWayExpansion=0x6A,
	DisconnectCnt=0x6C, FalseCarrierCnt=0x6E,
	NWayTestReg=0x70,
	RxCnt=0x72,		/* packet received counter */
	CSCR=0x74,		/* chip status and configuration register */
	PhyParm1=0x78,TwisterParm=0x7c,PhyParm2=0x80,	/* undocumented */
	/* from 0x84 onwards are a number of power management/wakeup frame
	 * definitions we will probably never need to know about.  */
};

enum RxEarlyStatusBits {
	ERGood=0x08, ERBad=0x04, EROVW=0x02, EROK=0x01
};
enum ChipCmdBits {
	CmdReset=0x10, CmdRxEnb=0x08, CmdTxEnb=0x04, RxBufEmpty=0x01, };
enum IntrMaskBits {
	SERR=0x8000, TimeOut=0x4000, LenChg=0x2000,
	FOVW=0x40, PUN_LinkChg=0x20, RXOVW=0x10,
	TER=0x08, TOK=0x04, RER=0x02, ROK=0x01
};
/* Interrupt register bits, using my own meaningful names. */
enum IntrStatusBits {
	PCIErr=0x8000, PCSTimeout=0x4000, CableLenChange= 0x2000,
	RxFIFOOver=0x40, RxUnderrun=0x20, RxOverflow=0x10,
	TxErr=0x08, TxOK=0x04, RxErr=0x02, RxOK=0x01,
};
enum TxStatusBits {
	TxHostOwns=0x2000, TxUnderrun=0x4000, TxStatOK=0x8000,
	TxOutOfWindow=0x20000000, TxAborted=0x40000000,
	TxCarrierLost=0x80000000,
};
enum RxStatusBits {
	RxMulticast=0x8000, RxPhysical=0x4000, RxBroadcast=0x2000,
	RxBadSymbol=0x0020, RxRunt=0x0010, RxTooLong=0x0008, RxCRCErr=0x0004,
	RxBadAlign=0x0002, RxStatusOK=0x0001,
};
enum MediaStatusBits {
	MSRTxFlowEnable=0x80, MSRRxFlowEnable=0x40, MSRSpeed10=0x08,
	MSRLinkFail=0x04, MSRRxPauseFlag=0x02, MSRTxPauseFlag=0x01,
};
enum MIIBMCRBits {
	BMCRReset=0x8000, BMCRSpeed100=0x2000, BMCRNWayEnable=0x1000,
	BMCRRestartNWay=0x0200, BMCRDuplex=0x0100,
};
enum CSCRBits {
	CSCR_LinkOKBit=0x0400, CSCR_LinkChangeBit=0x0800,
	CSCR_LinkStatusBits=0x0f000, CSCR_LinkDownOffCmd=0x003c0,
	CSCR_LinkDownCmd=0x0f3c0,
};
enum RxConfigBits {
	RxCfgWrap=0x80,
	Eeprom9356=0x40,
	AcceptErr=0x20, AcceptRunt=0x10, AcceptBroadcast=0x08,
	AcceptMulticast=0x04, AcceptMyPhys=0x02, AcceptAllPhys=0x01,
};
enum Config1Bits {
	VPDEnable=0x02,
};

uint32_t get_iobase(pci_cfg *cfg) {
    for (uint32_t i = 0; i < 6; i++) {
        if (cfg->base[i] != 0) {
            return cfg->base[i];
        }
    }

    return -1;
}
 
void cpuWriteIoApic(void *ioapicaddr, uint32_t reg, uint32_t value) {
   uint32_t volatile *ioapic = (uint32_t volatile *)ioapicaddr;
   ioapic[0] = (reg & 0xff);
   ioapic[4] = value;
}

void get_packet() {
    // Wait for packet
    while ((inb(iobase + CHIP_CMD) & RX_BUF_EMPTY) != 0);
    Debug::printf("iobase: 0x%x\n", iobase);

    Debug::printf("Packet length: 0x%x\n", *((uint16_t*)(rx_buffer + 1)));
    while(true);

    // Advance rx buffer
    Debug::printf("rx_buffer: 0x%x\n", (uint32_t) rx_buffer);
    rx_buffer += 64;
    outl(iobase + 0x30, (uintptr_t)rx_buffer);
    outb(iobase + 0x37, 0x0C);
}

void listen() {
    while (true) {
        get_packet();
        Debug::printf("Received packet\n");
    }
}

extern "C" void network_int(uint32_t *reg) {
    Debug::printf("Interrupt triggered\n");

    // Acknowledge packet
    // outb(iobase + 0x3E, 0x05); // bits 7..0
    // outb(iobase + 0x3E + 1, 0x00); // bits 15..8
}

void Network::init() {
    // Find network card on PCI
    Interrupts::disable();
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
    Debug::printf("rx_buffer: 0x%x\n", rx_buffer);
    bzero(rx_buffer, RX_BUF_SIZE);
    outl(iobase + 0x30, (uint32_t)rx_buffer);

    // Set IMR + ISR
    outb(iobase + 0x3C, 0xFF); // bits 7..0
    outb(iobase + 0x3C + 1, 0xFF); // bits 15..8

    // Configure receiving buffer
    outl(iobase + 0x44, 0xf | (1 << 7));

    // Enable receiving and transmitting
    // outb(CmdRxEnb, iobase + ChipCmd);
	// outl(((RX_FIFO_THRESH << 13 ) | (RX_BUF_LEN_IDX << 11) | (RX_DMA_BURST << 8) | AcceptBroadcast | AcceptMulticast | AcceptMyPhys), iobase + RxConfig);
	// outl(0xffffffffUL, iobase + MAR0 + 0);
	// outl(0xffffffffUL, iobase + MAR0 + 4);
	// outl(((TX_DMA_BURST << 8 ) | (TX_IPG << 24 )), iobase + TxConfig);
    outb(iobase + 0x37, 0x0C);

    // ISR Handler
    // outb(iobase + 0x3E, 0x01); // bits 7..0
    // outb(iobase + 0x3E + 1, 0x00); // bits 15..8

    listen();

    // Register interrupt handler
    pci_read_irq(network_card);
    Debug::printf("irq: 0x%x\n", network_card->irq);

    // for (uint32_t i = 0; i < 100; i++) {
    //     // AtomicPtr<uint32_t> irq_register = (uint32_t*) (kConfig.ioAPIC + 4 * i);
    //     // // Debug::printf("ioAPIC register: 0x%x\n", (uint32_t) irq_register);
    //     // irq_register.set(0 << 16 | NETWORK_INTERRUPT_VECTOR);
    //     // uint32_t *irq_register = (uint32_t*) (kConfig.ioAPIC + 4 * i);
    //     // *irq_register = (0 << 16 | NETWORK_INTERRUPT_VECTOR);
    //     cpuWriteIoApic((void*) kConfig.ioAPIC, i, 0 << 16 | NETWORK_INTERRUPT_VECTOR);
    // }

    // // for (uint32_t i = 0; i < 4096; i++) {
    // //     *((char*) (kConfig.ioAPIC + i)) = 60;
    // // }

    // Debug::printf("irq_register: 0x%x\n", *irq_register);
    // *((char*) kConfig.ioAPIC) = 0x10 + 2 * network_card->irq;
    // Debug::printf("ioAPIC reg 1: 0x%x\n", *((char*) kConfig.ioAPIC + 1));

    uint8_t mask = inb(PORT_INT_MASK_S);
    mask &= ~(1 << (network_card->irq - 8));
    outb(PORT_INT_MASK_S, mask);
    // cpuWriteIoApic((void*) kConfig.ioAPIC, 0x10 + 2 * network_card->irq, 0 << 16 | NETWORK_INTERRUPT_VECTOR);
    // cpuWriteIoApic((void*) kConfig.ioAPIC, 0x10 + 2 * 11 + 1, 0 << 16 | NETWORK_INTERRUPT_VECTOR);

    IDT::interrupt(32 + network_card->irq, (uint32_t) network_int_);
    sti();
}