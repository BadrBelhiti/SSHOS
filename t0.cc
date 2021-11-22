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
            for (uint32_t i=0; i<cnt; i++) {
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

   //  get "/hello"
   auto hello = fs->find(root,"hello");
   CHECK(root->number != hello->number);
   show("/hello",hello,true);

   // get "/goodbye"
   auto goodbye = fs->find(root,"goodbye");
   CHECK(hello->number != goodbye->number);
   CHECK(root->number != goodbye->number);
   show("/goodbye",goodbye,true);

   // get "/not_there"
   auto not_there = fs->find(root,"not_there");
   show("/not_there",not_there,true);

   // get "/fotunes"
   auto fortunes = fs->find(root,"fortunes");
   show("/fotunes",fortunes,false);
   {
       Debug::printf("***      ");
       uint32_t start = 19961;
       uint32_t count = 52;
       for (uint32_t i = 0; i<count; i++) {
           char c;
           fortunes->read(start+i,c);
           Debug::printf("%c",c);
       }
       Debug::printf("\n");
   }

   // get "/."
   show("/.",fs->find(root,"."),true);
   CHECK(root->number == fs->find(root,".")->number);

   // get "/.."
   show("/..",fs->find(root,".."),true);
   CHECK(root->number == fs->find(root,"..")->number);

   // get "/data"
   auto data = fs->find(root,"data");
   show("/data",data,true);
   CHECK(root->number == fs->find(data,"..")->number);
   CHECK(data->number == fs->find(data,".")->number);
   CHECK(data->number != root->number);

   // get "/data/data.txt"
   auto data_txt = fs->find(data,"data.txt");
   show("/data/data.txt",data_txt,true);
   {
       uint32_t start = 17;
       auto sz = data_txt->size_in_bytes();
       CHECK(sz >= start);
       auto buffer = new char[sz - start + 1];
       for (uint32_t n = 4; n<sz-start; n++) {
           buffer[n] = 0;
           auto cnt = data_txt->read_all(start,n,buffer);
           CHECK(cnt == n);
           Debug::printf("%s\n",buffer);
       }
       delete[] buffer;
   }

   // get "/data/panic.txt"
   show("/data/panic.txt",fs->find(data,"panic.txt"),true);
}

