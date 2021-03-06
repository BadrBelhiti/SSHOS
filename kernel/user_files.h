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
        bool consoleDevice;

        OpenFile(uint32_t fd, bool readable, bool writeable, bool consoleDevice) : ref_count(0) {
            this->offset = 0;
            this->fd = fd;
            this->readable = readable;
            this->writeable = writeable;
            this->consoleDevice = consoleDevice;
        }

        int read(void *buffer, uint32_t n) {
            if (!this->readable) {
                // Debug::printf("not readable\n");
                return -1;
            }

            uint32_t to_read = K::min(vnode->size_in_bytes() - offset, n);
            uint32_t actually_read = vnode->read_all(offset, to_read, (char*) buffer);
            // Debug::printf("to read: %d, actually read: %d\n", to_read, actually_read);
            offset += actually_read;
            return actually_read;
        }

        int write(void *buffer, uint32_t n) {
            auto me = gheith::current();
            if (!this->writeable) {
                return -1;
            }

            if (consoleDevice && writeable) { // write to shell
                gheith::Redirection *redirect_data = me->redirection;
                if (redirect_data == nullptr) {
                    for (uint32_t i = 0; i < n; i++) {
                        me->shell->printf("%c", ((char*) buffer)[i]);
                    }
                } else {
                    redirect_data->output_file->write_all(redirect_data->offset, (char*) buffer, n);
                }
                me->shell->refresh();
            } else { // write to file
                vnode->write_all(0, (char *) buffer, n);
            }

            return n;
        }
};