#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "stdint.h"

#define RX_BUF_SIZE 4096

class Network {
    private:
        char *rx_buffer;
        uint32_t iobase;
    public:
        Network();
};

#endif