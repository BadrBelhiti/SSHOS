#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "stdint.h"

#define RX_BUF_SIZE 8 * 1024 + 1500 + 16

class Network {
    private:
        char *rx_buffer;
        uint32_t iobase;
    public:
        Network();
};

#endif