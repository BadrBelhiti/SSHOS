#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "stdint.h"

#define RX_BUF_SIZE 8 * 1024 + 1500 + 16
#define CHIP_CMD 0x37
#define RX_BUF_EMPTY 0x01
#define PORT_INT_MASK_M 0x21
#define PORT_INT_MASK_S 0xA1
#define NETWORK_INTERRUPT_VECTOR 0x80
#define TX_FIFO_THRESH	256	/* In bytes, rounded down to 32 byte units. */
#define RX_FIFO_THRESH	4	/* Rx buffer level before first PCI xfer.  */
#define RX_DMA_BURST	4	/* Maximum PCI burst, '4' is 256 bytes */
#define TX_DMA_BURST	4	/* Calculate as 16<<val. */
#define TX_IPG		3	/* This is the only valid value */
#define RX_BUF_LEN_IDX	0	/* 0, 1, 2 is allowed - 8,16,32K rx buffer */
#define RX_BUF_LEN ( (8192 << RX_BUF_LEN_IDX) )
#define RX_BUF_PAD 4

namespace Network {
    void init();
};



#endif