#include "ide.h"
#include "ext2.h"
#include "shared.h"
#include "libk.h"
#include "ide.h"

void show(const char* name, Shared<Node> node, bool show) {
    Debug::printf("*** looking at %s\n",name);

    if (node == nullptr) {
        Debug::printf("***      does not exist\n");
        return;
    } 

    if (node->is_dir()) {
        Debug::printf("***      is a directory\n");
        Debug::printf("***      contains %d entries\n",node->entry_count());
        Debug::printf("***      has %d links\n",node->n_links());
        Debug::printf("***      Directory Size: %d\n", node->size_in_bytes());
    } else if (node->is_symlink()) {
        Debug::printf("***      is a symbolic link\n");
        auto sz = node->size_in_bytes();
        Debug::printf("***      link size is %d\n",sz);
        auto buffer = new char[sz+1];
        buffer[sz] = 0;
        node->get_symbol(buffer);
        Debug::printf("***       => %s\n",buffer);
    } else if (node->is_file()) {
        Debug::printf("***      is a file\n");
        auto sz = node->size_in_bytes();
        Debug::printf("***      contains %d bytes\n",sz);
        Debug::printf("***      has %d links\n",node->n_links());
        if (show) {
            auto buffer = new char[sz+1];
            buffer[sz] = 0;
            auto cnt = node->read_all(0,sz,buffer);
            CHECK(sz == cnt);
            CHECK(K::strlen(buffer) == cnt);
            // can't just print the string because there is a 1000 character limit
            // on the output string length.
            for (uint32_t i = 0; i < cnt; i++) {
                Debug::printf("%c",buffer[i]);
            }
            delete[] buffer;
            Debug::printf("\n");
        }
    } else {
        Debug::printf("***    is of type %d\n",node->get_type());
    }
}

/* Called by one CPU */
void kernelMain(void) {

    // IDE device #1
    auto ide = Shared<Ide>::make(1);
    
    // We expect to find an ext2 file system there
    auto fs = Shared<Ext2>::make(ide);

    Debug::printf("*** block size is %d\n",fs->get_block_size());
    Debug::printf("*** inode size is %d\n",fs->get_inode_size());
   
    // get "/"
    auto root = fs->root;
    show("/",root,true);

    Debug::printf("Number entries found: %d\n", root->entry_count());

    char buffer[12];
    ::memcpy(buffer, "justin text", 11);
    buffer[11] = 0;

    fs->createNode(root, "justin", ENTRY_FILE_TYPE);
    Shared<Node> foundNode = fs->find(root, "justin");
    foundNode->write_all(0, buffer, 11);
    show("justin", foundNode, true);

    Debug::printf("Number entries before deleting: %d\n", root->entry_count());

    foundNode->deleteNode(root);
    
    Debug::printf("Number entries after deleting: %d\n", root->entry_count());

    fs->createNode(root, "new_file", ENTRY_FILE_TYPE);
    foundNode = fs->find(root, "new_file");
    char newBuffer[41];
    ::memcpy(newBuffer, "this is new text that we are writing!!!", 39);
    newBuffer[39] = 0;
    
    Debug::printf("\n\nWriting to new file\n");
    foundNode->write_all(0, newBuffer, 39);
    show("new_file", foundNode, true);
    Debug::printf("Number entries after creating new file: %d\n", root->entry_count());
}