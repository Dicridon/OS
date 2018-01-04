#include "common.h"
static const unsigned char group_index[GROUP_SIZE] = {
     0x80,
     0x40,
     0x20,
     0x10,
     0x08,
     0x04,
     0x02,
     0x01
};


/*define global variables here*/

/*
 Use linear table or other data structures as you need.
 
 example:

 unsigned long long block_bit_map[];
 Your dentry index structure, and so on.
 
 
 //keep your root dentry and/or root data block
 //do path parse from your filesystem ROOT<@mount point>
 */



// super block, inode_bitmap, dblock_bitmap and inode_table should take up to
// 8256 blocks in a disk.
// the 8257th(count form zero) block is used as a spare superblock
// So data block begins at 8258th block(count from zero)


#define INODE_BITMAP_BLOCKS ((GROUP_NUM)/SECTOR_SIZE)                       // 32
#define DBLOCK_BITMAP_BLOCKS ((GROUP_NUM)/SECTOR_SIZE)                      // 32
#define INODE_BLOCKS  ((INODE_NUM * sizeof(struct inode_t))/SECTOR_SIZE)    // 8192
#define ROOT_INODE (0)
#define SUPERBLOCK (0)


#define INODE_BITMAP_BASE 1
#define DBLOCK_BITMAP_BASE (INODE_BITMAP_BASE+INODE_BITMAP_BLOCKS)          // 33
#define INODE_BASE (DBLOCK_BITMAP_BASE+DBLOCK_BITMAP_BLOCKS)                // 65
#define SPSUPERBLOCK (INODE_BASE + INODE_BLOCKS)                            // 8257
#define ROOT_DIR (SPSUPERBLOCK+1)                                           // 8258
#define DBLOCK_BASE (ROOT_DIR+5)                                            // 8263

/***************************************************************************
 ***************************************************************************
 ** disk layout is as follow,   
 ** each block stores 128 inodes, 
 ** the first inode and first data block are reserved for root directory. 
 ** all address is actually indexes of blocks
 ***************************************************************************
 *************************************************************************** 

  |             |
  | super block |
  |             |
  0

  |              |              |              |              |
  | inode_bitmap | inode_bitmap | ............ | inode_bitmap |
  |              |              |              |              |
  1              2              3              32

  |               |               |               |               |
  | dblock_bitmap | dblock_bitmap | ............. | dblock_bitmap |
  |               |               |               |               |
  33              34              35             64

  |        |        |        |        |
  | inodes | inodes | ...... | inodes | 
  |        |        |        |        |
  65       66       67       8256

  |             |
  | super block |
  |             |
  8257

  |  root  |  root  |  root  |  root  |       |        |
  | dblock | dblock | dblock | dblock | ..... | dblock |
  |        |        |        |        |       |        |
  8258    8259      8260     8261     8262

 **************************************************************************
 **************************************************************************/




struct inode inode_table[INODE_NUM];          // 8192 blocks in total
unsigned char inode_bitmap[GROUP_NUM];        // 32 blocks in total
unsigned char dblock_bitmap[GROUP_NUM];       // 32 blocks in total
struct dentry rootdir;
struct superblock_t glo_superblock;
struct file_info fd_table[MAX_OPEN_FILE] ;


static void pathref(const char* path){
    char* innerpath = path;
    char* token;
    token = strtok(innerpath, "/");
}


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


static void readdirfile(struct file_inode* fip, unsigned int dirsize, unsigned int base, unsigned int ref){
    unsigned int blocks = dirsize / SECTOR_SIZE;
    blocks += (blocks % SECTOR_SIZE == 0);
    unsigned char buffer[SECTOR_SIZE];

    for(unsigned int i = 0; i < (blocks <= 4) ? blocks : 4; i++){
	device_read_sector(buffer, base + 1);
	memcpy((char*)fip+i*SECTOR_SIZE, buffer, (dirsize > SECTOR_SIZE) ? SECTOR_SIZE : dirsize);
	dirsize -= SECTOR_SIZE;
    }
    if(blocks <= 4)
	return;


    // read the blocks in reference blocks
    blocks -= 4;
    unsigned int references[SECTOR_SIZE/4];
    device_read_sector(buffer, ref);
    memcpy(references, buffer, SECTOR_SIZE);
    
    for(unsigned int i = 0; i < (blocks <= 4) ? blocks : 4; i++){
	device_read_sector(buffer, references[i]);
	memcpy(fip, buffer, (dirsize > SECTOR_SIZE) ? SECTOR_SIZE : dirsize);
	dirsize -= SECTOR_SIZE;
    }
}


static int mount(struct superblock_t *sb){
    // read superblock, two bitmap, inodes
    // initialize fd table
    // read root directory

    unsigned char buffer[SECTOR_SIZE];
    memcpy(sb, buffer, sizeof(struct superblock_t));
    for(int i = 0; i < MAX_OPEN_FILE; i++){
	fd_table[i].fd = i;
	fd_table[i].flag = 0;
	fd_table[i].node = NULL;
    }

    rootdir.ino = 0;
    rootdir.dir_size = sb->root_dir_size;

    // read the whole directory in memory;
    // each block stores 64 entries;
    rootdir.fi = (struct file_inode*)malloc(rootdir.dir_size);
    readdirfile(rootdir.fi, rootdir.dir_size, ROOT_DIR, ROOT_DIR+4);

    // read superblock
    device_read_sector(buffer, SUPERBLOCK);
    memcpy(sb, buffer, sizeof(struct superblock_t));

    // read inode bitmap
    unsigned int i;
    for(i = 0; i < INODE_BITMAP_BLOCKS; i++){
	device_read_sector(inode_bitmap+i*SECTOR_SIZE, sb->inode_bitmap_base+i);
    }

    // read data block bitmap
    for(i = 0; i < DBLOCK_BITMAP_BLOCKS; i++){
	device_read_sector(dblock_bitmap+i*SECTOR_SIZE, sb->dblock_bitmap_base+i);
    }

    // read inodes
    for(i = 0; i < INODE_BLOCKS; i++){
	device_read_sector((unsigned char*)inode_table + i*SECTOR_SIZE, sb->inode_table_base+i);
    }
}

static int mkfs(struct superblock_t *sb){
    // initialize in-memeory super block
    // make space for two bitmaps and inodes
    // write it to disk using device_write_sector
    sb->magic = 1;
    sb->num_dblock = INODE_NUM;                            // 1048576
    sb->num_inodes = INODE_NUM;     		           // 1048576
    sb->root_dir = ROOT_DIR;        		           // 8258
    sb->root_dir_inode = ROOT_INODE;		           // 0
    sb->root_dir_size = 2 * sizeof(struct file_inode);     // . and ..
    sb->inode_table_base = INODE_BASE; 		           // 65
    sb->dblock_base = DBLOCK_BASE;     		           // 8262
    sb->inode_bitmap_base = INODE_BITMAP_BASE;	           // 1
    sb->dblock_bitmap_base = DBLOCK_BITMAP_BASE;           // 33   

    // initialize root directory
    inode_table[ROOT_INODE].inode->mode = DIR_T;
    inode_table[ROOT_INODE].inode->nlinks = 1;
    inode_table[ROOT_INODE].inode->size = 0;
    for(int i = 0; i < 4; i++){
	inode_table[ROOT_INODE].inode->direct_pointer[i] = i + ROOT_DIR;
    }
    inode_table[ROOT_INODE].inode->one_level_pointer = -1;


    // write superblock
    device_write_sector((unsigned char*)sb, SUPERBLOCK);  

    // write inode bitmap
    unsigned int i;
    for(i = 0; i < INODE_BITMAP_BLOCKS; i++){
	device_write_sector(inode_bitmap+i*SECTOR_SIZE, sb->inode_bitmap_base+i);
    }

    // write data block bitmap
    for(i = 0; i < DBLOCK_BITMAP_BLOCKS; i++){
	device_write_sector(dblock_bitmap+i*SECTOR_SIZE, sb->dblock_bitmap_base+i);
    }

    // wrtie inodes
    unsigned char* cp_inode = (unsigned char*)inode_table;
    for(i = 0; i < INODE_BLOCKS; i++){
	device_write_sector(cp_inode + i*SECTOR_SIZE, sb->inode_table_base+i);
    }

    // write spare superblock
    device_write_sector((unsigned char*)sb, SPSUPERBLOCK);
    device_flush();
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
    struct superblock_t ssp;
    unsigned char buffer[SECTOR_SIZE];

    device_open("DISK");
    device_read_sector(buffer, SUPERBLOCK);
    memcpy(&glo_superblock, buffer, sizeof(struct superblock_t));

    device_read_sector(buffer, SPSUPERBLOCK);
    memcpy(&ssp, buffer, sizeof(struct superblock_t));
    if(glo_superblock.magic != 1){
	if(ssp.magic != 1)     // superblock and spare superblock are both broken, mkfs
	    mkfs(&glo_superblock);
	else{                  // recover data from spare superblock
	    memcpy(buffer, &ssp, sizeof(struct superblock_t));
	    device_write_sector(buffer, SUPERBLOCK);
	    device_flush();
	}
    }

    mount(&glo_superblock);
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

}
