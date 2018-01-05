#ifndef P6_COMMON
#define P6_COMMON

#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Maybe you need the pthread locks or speedup by multi-threads or background GC in task2
// check if your $(CC) add -lfuse -pthread to $(CFLAGS) automatically, if not add them manually.
#include <pthread.h>

#include "disk.h"
#include "logging.h"



// each group consists eight blocks
#define GROUP_SIZE 8
// each block is 4KB
#define BLOCK_SIZE 4096

// 131072 groups track 4GB space in total, that is 1048576 inodes in total
// , so I used 128KB space to manage  4GB space

#define K ((unsigned long)1024)
#define M (((unsigned long)1024)*K)
#define G (((unsigned long)1024)*M)

#define GROUP_NUM (((4*G)/(4*K))/GROUP_SIZE)     // 131072
#define INODE_NUM ((GROUP_SIZE) * (GROUP_NUM))   // 1M inodes
#define MAX_OPEN_FILE 128

typedef enum {
    FILE_T,
    DIR_T,
    SYMLINK_T,
} node_type;

/*   on-disk  data structure   */
struct superblock_t{
    // complete it
    unsigned int magic;
    unsigned int root_dir;         // address of root directory
    unsigned int root_dir_inode;
    unsigned int root_dir_size;    // how many blocks root directory uses
    unsigned int num_dblock;       // number of data blocks
    unsigned int num_inodes;       // number of i-nodes
    unsigned int inode_table_base;      // base address of i-node table
    unsigned int dblock_base;      // base address of data blocks
    unsigned int inode_bitmap_base;
    unsigned int dblock_bitmap_base;
    // this struct is well aligned
};


// size of struct inode_t is 32 bytes
struct inode_t{
    // complete it
    node_type mode;
    unsigned int size;           // enough to hold 4 GB file
    unsigned int nlinks;         // hard links and symbol links
    unsigned int direct_pointer[4];    // pointers to data block, each manage 4KB space
    unsigned int one_level_pointer;   // two level addressing will give you a char pointer
};


struct file_inode{
    char file_name[60];
    unsigned int ino;
};

struct dentry{
    // complete it
    int ino;
    unsigned int dir_size;
    struct file_inode* fi;
};

/*  in-memory data structure   */

struct superblock{
    struct superblock_t *sb;
    // Add what you need, Like locks
    // thinking
    pthread_mutex_t superblock_lock;
};

struct inode{
    struct inode_t *inode;
    // Add what you need, Like locks
    // thinking
    pthread_mutex_t inode_lock;
};
 
/*Your file handle structure, should be kept in <fuse_file_info>->fh
 (uint64_t see fuse_common.h), and <fuse_file_info> used in all file operations  */


struct file_info{
    // complete it
    struct inode_t *node;
    unsigned int fd;         // maybe we may use inode number
    int allocated;
    int flag;
};


//Interf.  See "fuse.h" <struct fuse_operations>
//You need to implement all the interfaces except optional ones

//dir operations
int p6fs_mkdir(const char *path, mode_t mode);
int p6fs_rmdir(const char *path);
int p6fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo);  
// int p6fs_opendir(const char *path, struct fuse_file_info *fileInfo);//optional
// int p6fs_releasedir(const char *path, struct fuse_file_info *fileInfo);//optional
// int p6fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo);//optional

//file operations
int p6fs_mknod(const char *path, mode_t mode, dev_t dev);
int p6fs_symlink(const char *path, const char *link);
int p6fs_link(const char *path, const char *newpath);
int p6fs_unlink(const char *path);
// int p6fs_readlink(const char *path, char *link, size_t size);//optional

int p6fs_open(const char *path, struct fuse_file_info *fileInfo); 
int p6fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
int p6fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo);
int p6fs_truncate(const char *path, off_t newSize);
// int p6fs_flush(const char *path, struct fuse_file_info *fileInfo);//optional
// int p6fs_fsync(const char *path, int datasync, struct fuse_file_info *fi);//optional
int p6fs_release(const char *path, struct fuse_file_info *fileInfo);


int p6fs_getattr(const char *path, struct stat *statbuf);
// int p6fs_utime(const char *path, struct utimbuf *ubuf);//optional
// int p6fs_chmod(const char *path, mode_t mode); //optional
// int p6fs_chown(const char *path, uid_t uid, gid_t gid);//optional

int p6fs_rename(const char *path, const char *newpath);
int p6fs_statfs(const char *path, struct statvfs *statInfo);
void* p6fs_init(struct fuse_conn_info *conn);
// void p6fs_destroy(void* private_data);//optional









/// TODO list
// [    ]p6fs_mkdir
// [TODO]p6fs_rmdir
// [    ]p6fs_readdir
// [TODO]p6fs_mknod
// [TODO]p6fs_symlink
// [TODO]p6fs_link
// [TODO]p6fs_unlink
// [    ]p6fs_open
// [TODO]p6fs_read
// [TODO]p6fs_write
// [TODO]p6fs_truncate
// [TODO]p6fs_release
// [    ]p6fs_getattr
// [TODO]p6fs_rename
// [TODO]p6fs_statfs
// [    ]p6fs_init
#endif
