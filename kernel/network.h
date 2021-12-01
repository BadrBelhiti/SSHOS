#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "stdint.h"

#define RX_BUF_SIZE 8 * 1024 + 1500 + 16
#define CHIP_CMD 0x37
#define RX_BUF_EMPTY 0x01

class Network {
    private:
        char *rx_buffer;
        uint32_t iobase;
    public:
        Network();
        void get_packet();
        void listen();
        void network_int(uint32_t *reg);
};

#endif