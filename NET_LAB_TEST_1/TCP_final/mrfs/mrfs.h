#include <iostream>
#include <fstream>
#include <random>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

class mrfs {
private:
    class block {
        char data[256];
    };
    block *FS;
    int _key;
    bool init;
    class superblock {
    public:
        int block_count;
        int& size;
        int& max_inodes;
        int& used_inodes;
        int* inodes;
        int& max_blocks;
        int& used_blocks;
        int* blocks;
        superblock();
        superblock(block *FS);  //NOLINT
        superblock& operator=(block* FS);
    };
    superblock sb;
    class indexnode {
    public:
        int filetype;
        int filesize;
        time_t lastModified;
        time_t lastRead;
        int owner;
        int acPermissions;
        int direct[8];
        int indirect;
        int doubleindirect;
    };
    indexnode *inode;
    inline int reqblock() {
        if(sb.used_blocks==sb.max_blocks) return -1;
        for(int i=0;i<sb.max_blocks;i++)
            if(sb.blocks[i]==0) return i;
        return -1;
    }
    inline int16_t reqinode() {
        if(sb.used_inodes==sb.max_inodes) return -1;
        for(int16_t i=0;i<sb.max_inodes;i++)
            if(sb.inodes[i]==0) return i;
        return -1;
    }
    class dirlist {
    public:
        char name[30];
        int16_t inode;
    };
    int16_t curdir;
    class blocklist {
    public:
        block **list;
        int *blist;
        int numblocks;
        blocklist();
        blocklist(block* FS, indexnode& inode);
        ~blocklist();
        inline block* operator[](int i) {
            return list[i];
        }
    };

public:
    const int &key;
    inline void print() {
        using std::cout;
        using std::endl;
        cout<<"Key: "<<key<<endl;
        cout<<"Size: "<<sb.size<<endl;
        cout<<"Max Inodes: "<<sb.max_inodes<<endl;
        cout<<"Used Inodes: "<<sb.used_inodes<<endl;
        cout<<"Max Blocks: "<<sb.max_blocks<<endl;
        cout<<"Used Blocks: "<<sb.used_blocks<<endl;
        cout<<"Super Blocks: "<<sb.block_count<<endl;
        cout<<"FS: "<<FS<<endl;
        cout<<"inode: "<<inode<<endl;
    }

    mrfs();
    mrfs(const int& key);   //NOLINT
    mrfs(const mrfs& other);
    mrfs& operator=(const int& key);
    mrfs& operator=(const mrfs& other);
    ~mrfs();

    int create_myfs(int size);
    int copy_pc2myfs(const char *source, const char *dest);
    int copy_myfs2pc(const char *source, const char *dest) const;
    int rm_myfs(const char *filename);
    int showfile_myfs(const char *filename) const;
    int ls_myfs() const;
    int mkdir_myfs(const char *dirname);
    int chdir_myfs(const char *dirname);
    int rmdir_myfs(const char *dirname);
    int open_myfs(const char *filename, char mode);
    int close_myfs(int fd);
    int read_myfs(int fd, int nbytes, char *buff);
    int write_myfs(int fd, int nbytes, char *buff);
    int eof_myfs(int fd);
    int dump_myfs (const char *dumpfile) const;
    int restore_myfs (const char *dumpfile);
    int status_myfs() const;
    int chmod_myfs(const char *name, int mode);
    int append(indexnode& inode, void* data, int size);

};