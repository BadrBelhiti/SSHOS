#include "ext2.h"
#include "libk.h"
#include "threads.h"

class OpenFile {
    public:
        Shared<Node> vnode;
        Atomic<uint32_t> ref_count;
        uint32_t offset;
        uint32_t fd;
        bool readable;
        bool writeable;

        OpenFile(uint32_t fd, bool readable, bool writeable) : ref_count(0), offset(0), fd(fd), readable(readable), writeable(writeable) {}

        int read(void *buffer, uint32_t n) {
            if (!this->readable) {
                return -1;
            }

            uint32_t to_read = K::min(vnode->size_in_bytes() - offset, n);
            uint32_t actually_read = vnode->read_all(offset, to_read, (char*) buffer);
            offset += actually_read;
            return actually_read;
        }

        int write(void *buffer, uint32_t n) {
            if (!this->writeable) {
                return -1;
            }

            for (uint32_t i = 0; i < n; i++) {
                gheith::current()->shell->printf("%c", ((char*) buffer)[i]);
            }

            return n;
        }
};