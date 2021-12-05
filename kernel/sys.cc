#include "sys.h"
#include "stdint.h"
#include "idt.h"
#include "debug.h"
#include "machine.h"
#include "physmem.h"
#include "config.h"
#include "threads.h"
#include "ext2.h"
#include "user_files.h"
#include "barrier.h"
#include "vmm.h"
#include "future.h"
#include "libk.h"
#include "elf.h"
#include "keyboard.h"

#define MAX_SEMAPHORES 10

using namespace gheith;

bool is_user(uint32_t start, size_t nbytes) {
    uint32_t start_page = PhysMem::framedown(start);

    if (start_page < 0x80000000 || start_page == kConfig.ioAPIC || start_page == kConfig.localAPIC) {
        return false;
    }

    if (kConfig.ioAPIC >= start && kConfig.ioAPIC < start + nbytes) {
        return false;
    }

    if (kConfig.localAPIC >= start && kConfig.localAPIC < start + nbytes) {
        return false;
    }

    return true;
}

Shared<Node> find_by_absolute(const char* name) {
    // Assert we are given an absolute path
    if (name[0] != '/') {
        return Shared<Node>{nullptr};
    }

    Shared<Ext2> fs = current()->fs;
    char *curr_char = (char*) name;
    uint32_t curr_pos = 0;
    uint32_t curr_name_start = curr_pos;
    Shared<Node> curr_file = fs->root;
    ASSERT(curr_file != nullptr);

    // Skip root slash
    curr_char++;
    curr_pos++;
    curr_name_start++;

    while (true) {
        if (*curr_char == '/' || *curr_char == 0) {
            uint32_t name_length = curr_pos - curr_name_start;
            char dir_name[name_length + 1];
            memcpy(dir_name, curr_char - name_length, name_length);
            dir_name[name_length] = 0;
            curr_name_start = curr_pos + 1;
            curr_file = fs->find(curr_file, dir_name);
            
            if (curr_file == nullptr) {
                return Shared<Node>{nullptr};
            }

            if (*curr_char == 0) {
                break;
            }
        }

        curr_char++;
        curr_pos++;
    }

    return curr_file;
}

int exit(int status) {
    TCB *me = current();

    // Parent is waiting for it
    if (me->parent != nullptr) {
        ASSERT(me->exit != nullptr);
        me->exit->set(status);
    }
    
    // Clean up virtual memory
    uint32_t *pd = current()->pd;
    for (uint32_t pdi = 512; pdi < 1024; pdi++) {
        uint32_t pde = pd[pdi];
        if ((pde & 1) == 0) {
            continue;
        }

        uint32_t *pt = (uint32_t*) (pde & 0xFFFFF000);

        for (uint32_t pti = 0; pti < 1024; pti++) {
            uint32_t pte = pt[pti];

            if ((pte & 1) == 0) {
                continue;
            }

            uint32_t va = (pdi << 22) | (pti << 12);
            if (va == kConfig.ioAPIC || va == kConfig.localAPIC) {
                continue;
            }

            unmap(pd, va);
        }
    }

    stop();
    return 0;
}

int readShellLine(char *buf) {
    auto me = current();
    me->shell->refresh();

    char curKey = get_key();
    uint32_t index = 0;

    while (curKey != RETURN) {
        // ensure we have a valid key
        if (curKey == 0 || (curKey == BACKSPACE && index == 0)) {
            curKey = get_key();
            continue;
        }

        // show key in shell
        if (me->shell->handle_key(curKey)) {
            me->shell->refresh();
        }

        // update buffer
        if (curKey == BACKSPACE) {
            index--;
        } else {
            buf[index] = curKey;
            index++;
        }

        curKey = get_key();
    }

    // int numBytes = me->shell->cursor - startingCursor;
    // ::memcpy(buf, me->shell->buffer, numBytes);

    // return number of bytes read
    return (int) index;
}

int write(int fd, char* buf, size_t nbytes) {
    if (fd < 0 || fd >= 10) {
        return -1;
    }

    if (!is_user((uint32_t) buf, nbytes)) {
        return -1;
    }

    Shared<OpenFile> open_file = current()->open_files[fd];
    if (open_file == nullptr) {
        return -1;
    }

    return open_file->write(buf, nbytes);
}

int fork(uint32_t *kernel_stack) {
    TCB *me = current();

    // Check if process quota has been exceed
    uint32_t child_index = 0;
    while (child_index < 10 && me->children[child_index] != nullptr) {
        child_index++;
    }

    // No room for child
    if (child_index == 10) {
        return -1;
    }

    // Get user stack pointer and user eip
    uint32_t user_stack = kernel_stack[3];
    uint32_t eip = kernel_stack[0];

    // Get parent pd
    uint32_t *my_pd = current()->pd;

    // Create child pd
    uint32_t *pd = make_pd();

    // Copy virtual memory
    for (uint64_t addr = 0x80000000; addr < 0xFFFFFFFF; addr += PhysMem::FRAME_SIZE) {
        if (addr == kConfig.ioAPIC || addr == kConfig.localAPIC) {
            continue;
        }

        // Parent virtual memory
        uint32_t pdi = addr >> 22;
        uint32_t pde = my_pd[pdi];

        if ((pde & 1) == 0) {
            continue;
        }

        uint32_t *pt = (uint32_t*) (pde & 0xFFFFF000);
        ASSERT(pt != nullptr);
        uint32_t pti = (addr >> 12) & 0x3FF;
        uint32_t pte = pt[pti];

        // Check if page is present
        if ((pte & 1) == 0) {
            continue;
        }

        // Copy page
        uint32_t page = PhysMem::alloc_frame(false);
        if (page == -1U) {
            return -1;
        }
        memcpy((uint32_t*) page, (uint32_t*) addr, PhysMem::FRAME_SIZE);

        map(pd, addr, page);
    }

    // Create child thread
    childThread(current(), pd, child_index + 10, [user_stack, eip] {
        switchToUser(eip, user_stack, 0);
    });

    // Debug::printf("Created child with pid %d\n", child_index + 10);
    return child_index + 10;
}

int sem(uint32_t init) {
    TCB *me = current();

    // Add semaphore to list
    uint32_t index = 0;
    while (index < 10 && me->semaphores[index] != nullptr) {
        index++;
    }

    if (index == 10) {
        return -1;
    }

    // Create semaphore
    me->semaphores[index] = Shared<Semaphore>::make(init);

    return index + 20;
}

int up(int s) {
    TCB *me = current();
    uint32_t index = s - 20;

    if (index < 0 || index >= MAX_SEMAPHORES) {
        return -1;
    }

    // Check whether semaphore exists
    if (me->semaphores[index] == nullptr) {
        return -1;
    }

    // Call down() on semaphore
    me->semaphores[index]->up();
    return 0;
}

int down(int s) {
    TCB *me = current();
    // Debug::printf("Down called by pid %d\n", me->pid);
    uint32_t index = s - 20;

    if (index < 0 || index >= MAX_SEMAPHORES) {
        return -1;
    }

    // Check whether semaphore exists
    if (me->semaphores[index] == nullptr) {
        return -1;
    }

    // Call down() on semaphore
    me->semaphores[index]->down();
    return 0;
}

int close(int id) {
    TCB *me = current();
    // Case file descriptor
    if (id >= 0 && id < 10) {
        Shared<OpenFile> open_file = me->open_files[id];
        if (open_file == nullptr) {
            return -1;
        }

        me->open_files[id] = nullptr;
    }
    // Case child processes
    else if (id < 20) {
        TCB *curr_child = me->children[id - 10];

        // Child with id doesn't exist
        if (curr_child == nullptr) {
            return -1;
        }

        // Disown child
        me->children[id - 10] = nullptr;
        curr_child->parent = nullptr;
    }
    // Case semaphore
    else if (id < 30) {
        uint32_t index = id - 20;

        if (index < 0 || index >= MAX_SEMAPHORES) {
            return -1;
        }

        // Check whether semaphore exists
        if (me->semaphores[index] == nullptr) {
            return -1;
        }

        // Remove semaphore from list
        me->semaphores[index] = nullptr;
        return 0;
    }
    // Case error
    else {
        return -1;
    }

    return 0;
}

int shutdown(void) {
    Debug::shutdown();
    return 0;
}

int wait(int id, uint32_t *ptr) {
    if (id < 10 || id >= 20) {
        return -1;
    }

    TCB *me = current();

    // Find child
    TCB *curr_child = me->children[id - 10];

    // Check to see if child exists
    if (curr_child == nullptr) {
        return -1;
    }

    ASSERT(curr_child->exit != nullptr);
    *ptr = curr_child->exit->get();
    
    return 0;
}

int execl(const char* path, const char** args) {
    // Find program in disk
    Shared<Node> program_vnode = find_by_absolute(path);

    if (program_vnode == nullptr) {
        return -1;
    }

    // Start moving program args to last page
    char *cursor = (char*) 0xFFFFF000;

    // Zero out last page in memory to store args
    bzero(cursor, PhysMem::FRAME_SIZE);

    // Count number of args
    uint32_t num_args = 0;
    for (num_args = 0; args[num_args] != 0; num_args++);

    // Start char** pointer from top of page
    uint32_t *stack_args = (uint32_t*) (0xFFFFFFFF - 4 * num_args);

    // Copy args. String literals go at bottom of the page.
    for (uint32_t i = 0; args[i] != 0; i++) {
        char *curr_arg = (char*) args[i];
        K::strcpy(cursor, curr_arg);
        stack_args[i] = (uint32_t) cursor;
        cursor += (K::strlen(curr_arg) + 1);
    }

    

    // Zero out all of private memory EXCEPT FOR LAST PAGE
    uint32_t *pd = current()->pd;
    for (uint32_t addr = 0x80000000; addr < 0xFFFFF000; addr += PhysMem::FRAME_SIZE) {
        if (addr == kConfig.ioAPIC || addr == kConfig.localAPIC) {
            continue;
        }
        
        unmap(pd, addr);
    }

    // Delete semaphores and children
    TCB *me = current();
    for (uint32_t i = 0; i < 10; i++) {
        me->semaphores[i] = nullptr;
        me->children[i] = nullptr;
    }

    // Load program
    uint32_t eip = ELF::load(program_vnode);

    // Prepare user stack
    uint32_t user_esp = K::min(kConfig.localAPIC, kConfig.ioAPIC);
    uint32_t *user_stack = (uint32_t*) user_esp;
    user_stack[-2] = num_args;
    user_stack[-1] = (uint32_t) stack_args;
    user_esp -= 8;


    // Jump to program
    ASSERT(!Interrupts::isDisabled());
    switchToUser(eip, (uint32_t) user_esp, 0);

    return 0;
}

int open(const char* fn) {
    Shared<OpenFile> *open_files = current()->open_files;
    uint32_t available_id = 0;

    // Look for available file descriptor
    while (open_files[available_id] != nullptr && available_id < 10) {
        available_id++;
    }

    // Maximum open file count has been reached
    if (available_id == 10) {
        return -1;
    }

    // Find file
    Shared<Node> vnode;
    if (fn[0] != '/') {
        vnode = current()->fs->find(current()->dir_inode, fn);
    } else {
        vnode = find_by_absolute(fn);
    }

    // File doesn't exist
    if (vnode == nullptr) {
        return -1;
    }

    // Create open file and add it to process's open files
    open_files[available_id] = Shared<OpenFile>::make(available_id, true, true, false);


    // Initialize open file
    open_files[available_id]->vnode = vnode;
    return available_id;
}

int len(int fd) {
    if (fd < 3 || fd >= 10) {
        return -1;
    }

    Shared<OpenFile> open_file = current()->open_files[fd];
    if (open_file == nullptr) {
        return -1;
    }

    return open_file->vnode->size_in_bytes();;
}

int removeStructure(int fd, bool removeFromRoot) {
    if (fd >= 10 || fd < 0) {
        return -1;
    }

    Shared<OpenFile> open_file = current()->open_files[fd];
    if (open_file == nullptr) {
        return -1;
    }

    if (removeFromRoot) {
        open_file->vnode->deleteNode(current()->fs->root);
    } else {
        open_file->vnode->deleteNode(current()->dir_inode);
    }
    
    return 1;
}

int read(int fd, void* buffer, ssize_t n) {
    if (fd >= 10 || fd < 0) {
        return -1;
    }

    if (!is_user((uint32_t) buffer, n)) {
        return -1;
    }

    Shared<OpenFile> open_file = current()->open_files[fd];
    if (open_file == nullptr) {
        return -1;
    }

    return open_file->read(buffer, n);
}

int seek(int fd, uint32_t off) {
    if (fd < 0 || fd >= 10) {
        return -1;
    }

    Shared<OpenFile> open_file = current()->open_files[fd];
    if (open_file == nullptr) {
        return -1;
    }

    if (!open_file->readable) {
        return -1;
    }

    if (off < 0) {
        return -1;
    }

    open_file->offset = off;
    return open_file->offset;
}


int opendir(const char* fn) {
    Shared<OpenFile> *open_files = current()->open_files;
    uint32_t available_id = 0;

    // Look for available file descriptor
    while (open_files[available_id] != nullptr && available_id < 10) {
        available_id++;
    }

    // Maximum open file count has been reached
    if (available_id == 10) {
        return -1;
    }

    Shared<Node> vnode;
    // Find directory, same as findinng file
    if (fn[0] == '/') {
        vnode = find_by_absolute(fn);
    } else {
        vnode = current()->fs->find(current()->dir_inode, fn);
    }

    // Directory doesn't exist
    if (vnode == nullptr) {
        return -1;
    }

    ASSERT(vnode->is_dir()); // make sure this is actually a directory lol

    // Create open file and add it to process's open files
    open_files[available_id] = Shared<OpenFile>::make(available_id, true, false, false);


    // Initialize file node 
    open_files[available_id]->vnode = vnode;
    return available_id;
}

int readdir(int fd, char* buff_start, uint32_t max_size) {
    Shared<OpenFile> *open_files = current()->open_files;
    Shared<Node> dir_node = open_files[fd]->vnode;
    
    bzero(buff_start, max_size);
    dir_node->get_entry_names(buff_start, max_size);

    return 1;
}

int getcwd(char* buff) {
    TCB *me = current();
    // Debug::printf("in cwd: %s\n", me->dir_name);
    memcpy(buff, me->dir_name, K::strlen(me->dir_name));
    // K::strcpy(buff, me->dir_name);
    buff[K::strlen(me->dir_name)] = '\0';
    // Debug::printf("%s, %s\n", me->dir_name, buff);
    return K::strlen(me->dir_name);
}

int chdir(const char* fn) {
    // open the directory first
    int fd = opendir(fn);
    if (fd == -1) return -1;
    TCB *tcb = current();

    Shared<OpenFile> *open_files = tcb->open_files;
    Shared<Node> node = open_files[fd]->vnode;

    tcb->dir_inode = node;
    bzero(tcb->dir_name, K::strlen((char*)fn));
    // bzero(tcb->dir_name, 10);
    int len = getcwd(tcb->dir_name);

    if (tcb->dir_inode != current()->fs->root) {
        tcb->dir_name[len] = '/';
        len++;
    }

    memcpy(tcb->dir_name + len, (char *) fn,  K::strlen((char*)fn));
    tcb->dir_name[K::strlen((char*)fn) +1] = '\0';



    // Debug::printf("new directory name: %s\n", tcb->dir_name);

    return fd;
}

int shell_theme(int theme) {
    TCB *me = current();

    if (theme != 0 && theme != 1) {
        return -1;
    }

    if (theme == 0) {
        me->shell->set_theme(WHITE_ON_BLACK);
    } else {
        me->shell->set_theme(WHITE_ON_BLUE);
    }

    return 0;
}

int touch(const char* fn) {
    TCB *me = current();
    bool res;
    if (fn[0] == '/') {
        res = me->fs->createNode(me->fs->root, (char *) fn, ENTRY_FILE_TYPE);
    } else {
        res = me->fs->createNode(me->dir_inode, (char *) fn, ENTRY_FILE_TYPE);
    }
    return res ? 1 : -1;
}

extern "C" int sysHandler(uint32_t eax, uint32_t *frame) {
    uint32_t *user_stack = (uint32_t*) frame[3];

    switch (eax) {
        // exit(int status_code)
        case 0:
            return exit(user_stack[1]);
        // write(int fd, void* buf, size_t nbytes)
        case 1:
            return write(user_stack[1], (char*) user_stack[2], user_stack[3]);
        // fork()
        case 2:
            return fork(frame);
        // sem(uint32_t init)
        case 3:
            return sem(user_stack[1]);
        // up(int s)
        case 4:
            return up(user_stack[1]);
        // down(int s)
        case 5:
            return down(user_stack[1]);
        // close(int id)
        case 6:
            return close(user_stack[1]);
        // shutdown()
        case 7:
            return shutdown();
        // wait(int id, uint32_t *ptr)
        case 8:
            return wait(user_stack[1], (uint32_t*) user_stack[2]);
        // execl(const char* path, const char* arg0, ....)
        case 9:
            return execl((char*) user_stack[1], (const char**) (user_stack + 2));
        // open(const char* fn)
        case 10:
            return open((char*) user_stack[1]);
        // len(int fd)
        case 11:
            return len(user_stack[1]);
        // read(int fd, void* buffer, size_t n)
        case 12:
            return read(user_stack[1], (void*) user_stack[2], user_stack[3]);
        // seek(int fd, off_t off)
        case 13:
            return seek(user_stack[1], user_stack[2]);
        // println(char *str)
        case 14:
            return shell_theme((int) user_stack[1]);
        // opendir(const char* fn)
        case 15:
            return opendir((const char*) user_stack[1]);
        // readdir(int fd)
        case 16:
            return readdir(user_stack[1], (char*) user_stack[2], user_stack[3]);

        case 17:
            return chdir((const char*) user_stack[1]);

        case 18:
            return touch((const char*) user_stack[1]);

        case 19:
            return readShellLine((char *) user_stack[1]);

        case 20:
            return removeStructure(user_stack[1], user_stack[2]);
        case 21:
            return getcwd((char*) user_stack[1]);
    }

    return 0;
}

extern "C" void debugSwitch(uint32_t pc, uint32_t userCS, uint32_t eflags, uint32_t esp, uint32_t userSS) {
    Debug::printf("Debug switch ~~ pc: 0x%x, userCS: 0x%x, eflags: 0x%x, esp: 0x%x, userSS: 0x%x\n", pc, userCS, eflags, esp, userSS);
}

void SYS::init(void) {
    IDT::trap(48,(uint32_t)sysHandler_,3);
}