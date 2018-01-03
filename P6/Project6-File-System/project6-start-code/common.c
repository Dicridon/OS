#include "common.h"

/*define global variables here*/

/*
 Use linear table or other data structures as you need.
 
 example:
 struct file_info[MAX_OPEN_FILE] fd_table;
 struct inode[MAX_INODE] inode_table;
 unsigned long long block_bit_map[];
 Your dentry index structure, and so on.
 
 
 //keep your root dentry and/or root data block
 //do path parse from your filesystem ROOT<@mount point>
 
 struct dentry* root;
 */


int p6fs_mkdir(const char *path, mode_t mode)
{
     /*do path parse here
      create dentry  and update your index*/
    
}

int p6fs_rmdir(const char *path)
{
    
}

int p6fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo)
{
    
}

//optional
//int p6fs_opendir(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_releasedir(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsyncdir(const char *path, int datasync, struct fuse_file_info *fileInfo)


//file operations
int p6fs_mknod(const char *path, mode_t mode, dev_t dev)
{
 /*do path parse here
  create file*/
}

//int p6fs_readlink(const char *path, char *link, size_t size)

int p6fs_symlink(const char *path, const char *link)
{
    
}

int p6fs_link(const char *path, const char *newpath)
{
    
}

int p6fs_unlink(const char *path)
{
    
}

int p6fs_open(const char *path, struct fuse_file_info *fileInfo)
{
 /*
  Implemention Example:
  S1: look up and get dentry of the path
  S2: create file handle! Do NOT lookup in read() or write() later
  */
    

    //assign and init your file handle
    struct file_info *fi;
    
    //check open flags, such as O_RDONLY
    //O_CREATE is tansformed to mknod() + open() by fuse ,so no need to create file here
    /*
     if(fileInfo->flags & O_RDONLY){
     fi->xxxx = xxxx;
     }
     */
    
    fileInfo->fh = (uint64_t)fi;

}

int p6fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    /* get inode from file handle and do operation*/
    
}

int p6fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    /* get inode from file handle and do operation*/
}

int p6fs_truncate(const char *path, off_t newSize)
{
    
}

//optional
//p6fs_flush(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
int p6fs_release(const char *path, struct fuse_file_info *fileInfo)
{
    /*release fd*/
}
int p6fs_getattr(const char *path, struct stat *statbuf)
{
    /*stat() file or directory */
}
/*
int p6fs_utime(const char *path, struct utimbuf *ubuf);//optional
int p6fs_chmod(const char *path, mode_t mode); //optional
int p6fs_chown(const char *path, uid_t uid, gid_t gid);//optional
*/

int p6fs_rename(const char *path, const char *newpath)
{
    
}

int p6fs_statfs(const char *path, struct statvfs *statInfo)
{
    /*print fs status and statistics */
}

void* p6fs_init(struct fuse_conn_info *conn)
{
    /*init fs
     think what mkfs() and mount() should do.
     create or rebuild memory structures.
     
     e.g
     S1 Read the magic number from disk
     S2 Compare with YOUR Magic
     S3 if(exist)
        then
            mount();
        else
            mkfs();
     */
    
    
    /*HOWTO use @return
     struct fuse_context *fuse_con = fuse_get_context();
     fuse_con->private_data = (void *)xxx;
     return fuse_con->private_data;
     
     the fuse_context is a global variable, you can use it in
     all file operation, and you could also get uid,gid and pid
     from it.
     
     */
    return NULL;
}
void p6fs_destroy(void* private_data)
{
    /*
     flush data to disk
     free memory
     */
    device_close();
    logging_close();
}


unsigned int path_ref(const char *path){

    char *duppath = strdup(path);
    char* token = strtok(duppath, "/");
    unsigned int current_dir;             // inode of current directory
    // from root directory
    if(duppath[0] == '/'){
	// do something
	current_dir = root_dir;
    }
    else{
	// from working directory
	current_dir = working_dir;
    }
    while(token){
	// do something
	
	// next token
	token = strtok(NULL, "/");
    }
    free(duppath);
}
