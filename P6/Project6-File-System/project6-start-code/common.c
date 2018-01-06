#include "common.h"

// #define FUSE_USE_VERSION 26

#define PARENT (1)
#define CHILD (0)


#define GI1 (0)
#define GI2 (1)
#define GI3 (2)
#define GI4 (3)
#define GI5 (4)
#define GI6 (5)
#define GI7 (6)
#define GI8 (7)

static const unsigned char group[GROUP_SIZE] = {
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

  |  root  |  root  |  root  |  root  |  root  |        |        |
  | dblock | dblock | dblock | dblock | dblock | ...... | dblock |
  |        |        |        |        |        |        |        |
  8258    8259      8260     8261     8262

 **************************************************************************
 **************************************************************************/


struct inode_t inode_table[INODE_NUM];          // 8192 blocks in total
unsigned char inode_bitmap[GROUP_NUM];        // 32 blocks in total
unsigned char dblock_bitmap[GROUP_NUM];       // 32 blocks in total
struct dentry rootdir;
struct dentry workdir;
struct superblock_t glo_superblock;
struct file_info fd_table[MAX_OPEN_FILE] ;

pthread_mutex_t bitmap_lock;  // lock both inode_bitmap and dblock_bitmap
pthread_mutex_t inode_lock;


// please allocate some memory to fip before calling function readdirfile
// this function will read whole dirctory into fip
// so specify directory size dirsize(bytes)
// pass the direct pointers and one-level pointer to base and ref
static void readdirfile(struct file_inode* fip, unsigned int dirsize, unsigned int* base, unsigned int ref){
    unsigned int blocks = dirsize / SECTOR_SIZE;
    blocks += (blocks % SECTOR_SIZE == 0);
    unsigned char buffer[SECTOR_SIZE];

    unsigned int limit = (blocks <= 4) ? blocks : 4;
    
    for(unsigned int i = 0, j = 0; i < limit && j < limit;){
	if(base[j] != 0xffffffff){
	    device_read_sector(buffer, base[i]);
	    memcpy((char*)fip+i*SECTOR_SIZE,
		   buffer,
		   (dirsize > SECTOR_SIZE) ? SECTOR_SIZE : dirsize);
	    dirsize -= SECTOR_SIZE;
	    i++;
	}
	j++;
    }
    if(blocks <= 4)
	return;

    // read the blocks in reference blocks
    int preblock = blocks;
    blocks -= 4;
    unsigned int references[SECTOR_SIZE/4];
    device_read_sector(buffer, ref);
    memcpy(references, buffer, SECTOR_SIZE);
    
    for(unsigned int i = 0, j = 0; i < blocks && j < blocks;){
	if(references[j] != 0xffffffff){
	    device_read_sector(buffer, references[i]);
	    memcpy((char*)fip+(i+preblock)*SECTOR_SIZE,
		   buffer,
		   (dirsize > SECTOR_SIZE) ? SECTOR_SIZE : dirsize);
	    dirsize -= SECTOR_SIZE;
	    i++;
	}
	j++;
    }
}


// DO allocate enough space for fip
static void writedirfile(struct file_inode* fip, unsigned int dirsize, unsigned int* base, unsigned int ref){
    unsigned int blocks = dirsize / SECTOR_SIZE;
    blocks += (blocks % SECTOR_SIZE == 0);
    unsigned char buffer[SECTOR_SIZE];

    unsigned int limit = (blocks <= 4) ? blocks : 4;
    
    for(unsigned int i = 0, j = 0; i < limit && j < limit;){
	if(base[j] != 0xffffffff){
	    memcpy(buffer,
		   (char*)fip+i*SECTOR_SIZE,
		   (dirsize > SECTOR_SIZE) ? SECTOR_SIZE : dirsize);
	    device_write_sector(buffer, base[j]);
	    dirsize -= SECTOR_SIZE;
	    i++;
	}
	j++;
    }
    if(blocks <= 4)
	return;

    // read the blocks in reference blocks
    int preblock = blocks;
    blocks -= 4;

    unsigned int references[SECTOR_SIZE/4];
    device_read_sector(buffer, ref);
    memcpy(references, buffer, SECTOR_SIZE);
    
    device_read_sector(buffer, ref);
    memcpy(references, buffer, SECTOR_SIZE);
    
    for(unsigned int i = 0, j = 0; i < blocks && j < blocks;){
	if(references[j] == 0xffffffff){
	    memcpy(buffer,
		   (char*)fip+(i+preblock)*SECTOR_SIZE,
		   (dirsize > SECTOR_SIZE) ? SECTOR_SIZE : dirsize);
	    device_read_sector(buffer, references[j]);
	    dirsize -= SECTOR_SIZE;
	    i++;
	}
	j++;
    }
    device_flush();
}


// DO NOT allocate space for *inode before calling this function
// fetch the inode_numth inode into struct inode_t *inode
static void getinode(struct inode_t *inode, unsigned int inode_num){
    unsigned char buffer[SECTOR_SIZE];
    device_read_sector(buffer, inode_num/128 + INODE_BASE);
    unsigned int offset = (inode_num % 128) * sizeof(struct inode_t);
    memcpy(inode, buffer+offset, sizeof(struct inode_t));
}


// check the value at position of bimap is 1 or 0
// bitmap has GROUP_NUM groups
// each group is one byte, controling 8 bits
// returning -1 indicating invalid position
static int bitmap_value_check(unsigned long position, const unsigned char *bitmap){
    if(position >= GROUP_NUM)
	return -1;
	
    int index = position / 8;
    int offset = position % 8;
    return bitmap[index] & group[offset];
}

// return the position of first zero bit in a bitmap
// returning indicating
static int bitmap_lookup(const unsigned char *bitmap){
    // inode_bitmap and dblock_bitmap have the same size
    int find = 0;
    unsigned long i;
    for(i = 0; i < INODE_NUM; i++){
	if(bitmap_value_check(i, bitmap) == 0){
	    find = 1;
	    break;
	}
    }
    return (find) ? i : -1;
}

static int write_bitmap_bit(unsigned long position, unsigned char *bitmap){
    if(position >= GROUP_NUM)
	return -1;
	
    int index = position / 8;
    int offset = position % 8;
    bitmap[index] |= group[offset];
}

static int clear_bitmap_bit(unsigned long position, unsigned char *bitmap){
    if(position >= GROUP_NUM)
	return -1;
	
    int index = position / 8;
    int offset = position % 8;
    bitmap[index] &= ~group[offset];
}
// 
static int update_inode_bitmap(void){
    unsigned int i;
    for(i = 0; i < INODE_BITMAP_BLOCKS; i++){
	if(device_write_sector((unsigned char*)inode_bitmap+i*SECTOR_SIZE, glo_superblock.inode_bitmap_base+i) != 1)
	    return -1;	    
    }
    device_flush();
    return 0;
}

static int update_dblock_bitmap(void){
    unsigned int i;
    for(i = 0; i < INODE_BITMAP_BLOCKS; i++){
	if(device_write_sector((unsigned char*)dblock_bitmap+i*SECTOR_SIZE, glo_superblock.dblock_bitmap_base+i) != 1)
	    return -1;	    
    }
    device_flush();
    return 0;
}

static int update_inodes(void){
    unsigned int i;
    for(i = 0; i < INODE_BLOCKS; i++){
	if(device_write_sector((unsigned char*)inode_table + i*SECTOR_SIZE, glo_superblock.inode_table_base+i) != 1)
	return -1;
    }
    device_flush();
    return 0;
}

static int update_superblock(void){
    unsigned char buffer[SECTOR_SIZE] = "";
    memcpy(buffer, &glo_superblock, sizeof(struct superblock_t));
    if(device_write_sector(buffer, SUPERBLOCK) != 1)
	return -1;
    if(device_write_sector(buffer, SPSUPERBLOCK) != 1)
	return -1;

    device_flush();
    return 0;
}

static int update_rootdir(void){
    struct inode_t inode;
    getinode(&inode, ROOT_INODE);
    rootdir.dir_size = inode.size;
    rootdir.fi = (struct file_inode*)realloc(rootdir.fi, rootdir.dir_size);
    readdirfile(rootdir.fi, rootdir.dir_size, inode.direct_pointer, inode.one_level_pointer);
}


// this function will upate in-memory root_dir
// write inode_table, inode_bitmap, dblock_bitmap, superblock to disk
static int update_metadata(int root){
    update_dblock_bitmap();
    update_inode_bitmap();
    update_inodes();
    if(root == 1){
	update_rootdir();
	glo_superblock.root_dir_size = rootdir.dir_size;
	update_superblock();
    }
    device_flush();
    return 0;
}

static int allocate_and_create_dir(struct inode_t *parent_inode, int parent_inode_num, struct file_inode *fip, char* filename){
    int items = parent_inode->size / sizeof(struct file_inode);
    int pindex;
    for(pindex = 0; pindex < items; pindex++){
	if(strcmp(filename, fip[pindex].file_name) == 0)
	    return -ENOENT;      // directory already exsits
    }

    // first deal with new inode
    int empty_inode_num = bitmap_lookup(inode_bitmap);
    if(empty_inode_num == -1)
	return -ENOENT;

    int slot;
    slot = bitmap_lookup(dblock_bitmap);
    if(slot == -1)
	return -ENOENT;

    for(int i = 0; i < 4; i++)
	inode_table[empty_inode_num].direct_pointer[i] = 0xffffffff;
    write_bitmap_bit(slot, dblock_bitmap);
    write_bitmap_bit(empty_inode_num, inode_bitmap);
    inode_table[empty_inode_num].mode = DIR_T;
    inode_table[empty_inode_num].nlinks = 2;
    inode_table[empty_inode_num].size = 2 * sizeof(struct file_inode);    
    inode_table[empty_inode_num].direct_pointer[0] = slot + DBLOCK_BASE;
    inode_table[empty_inode_num].one_level_pointer = 0xffffffff;

    // update parent directory
    strcpy(fip[pindex].file_name, filename);
    fip[pindex].ino = empty_inode_num;
    parent_inode->size += sizeof(struct file_inode);
    writedirfile(fip,
		 parent_inode->size,
		 parent_inode->direct_pointer,
		 parent_inode->one_level_pointer);

    // update inode_table
    inode_table[parent_inode_num].size = parent_inode->size;
        
    // write child directory
    struct file_inode new_fip[2] =  {
	{.file_name = ".", .ino = empty_inode_num},
	{.file_name = "..", .ino = parent_inode_num}
    };
    writedirfile(new_fip,
		 inode_table[empty_inode_num].size,
		 inode_table[empty_inode_num].direct_pointer,
		 inode_table[empty_inode_num].one_level_pointer);
}




static void nullptrcheck(void* p){
    if(p == NULL){
	printf("NULL POINTRE %s, %d", __FILE__, __LINE__);
	exit(-1);
    }
}

static int pathref(const char* path, int parent){
    char* innerpath = (char*)malloc(strlen(path) * sizeof(char));
    const struct dentry *currdir;
    char* token;
    int token_num = 0;
    int items;
    int i;
    int inode_num = ROOT_INODE;
    int find = 0;
    struct dentry tempdir;
    struct inode_t tempinode;
    
    nullptrcheck(innerpath);
    strcpy(innerpath, path);
    token = strtok(innerpath, "/");
    
    while(token){
	token_num++;
	token = strtok(NULL, "/");
    }
    free(innerpath);
    innerpath = (char*)malloc(strlen(path) * sizeof(char));
    nullptrcheck(innerpath);


    tempdir.fi = (struct file_inode*)malloc(1028*128*sizeof(struct file_inode));
    nullptrcheck(tempdir.fi);


    strcpy(innerpath, path);
    token = strtok(innerpath, "/");
//    if(*path == '/'){
    
    currdir = &rootdir;
    while(token != NULL){
	items = currdir->dir_size / sizeof(struct file_inode);
	for(i = 0; i < items; i++){
	    if(strcmp(currdir->fi[i].file_name, token) == 0){
		inode_num = currdir->fi[i].ino;
		find = 1;
		break;
	    }
	}
	    
	if(find == 0 && !parent){
	    inode_num = -ENOENT;
	    break;
	}
	// return parent dirctory
	else if(parent){
	    if(token_num == 1){
		inode_num = currdir->ino;
		break;
	    }
	}
		    
	getinode(&tempinode, inode_num);
	tempdir.dir_size = tempinode.size;
	tempdir.ino = inode_num;
	readdirfile(tempdir.fi,
		    tempdir.dir_size,
		    tempinode.direct_pointer,
		    tempinode.one_level_pointer);
	currdir = &tempdir;
	token = strtok(NULL, "/");
	token_num--;
	find = 0;
    }
	//  }

    free(innerpath);
    free(tempdir.fi);
    return inode_num;
}


// DO allocate sapce for char *name
static void getfilename(const char* path, char* filename){
    char* innerpath = (char*)malloc(strlen(path) * sizeof(char));
    strcpy(innerpath, path);

    char* name = path + strlen(path);
    while(*name != '/')
	name--;
    name++;
    // filename has at most 60 characters including ones null terminator
    if(strlen(name) > 59){
	*filename = '\0';
	return;
    }
    
    strcpy(filename, name);
}


static int remove_entry(struct inode_t *parent_inode, int parent_inode_num, const char* filename){
    struct file_inode *fip = (struct file_inode*)malloc(parent_inode->size);

    if(fip == NULL)
	return -ENOMEM;
    memset(fip, 0, parent_inode->size);
    // TODO return -ENOBUF in readdir
    readdirfile(fip, parent_inode->size,
		parent_inode->direct_pointer, parent_inode->one_level_pointer);

    int items = parent_inode->size / sizeof(struct file_inode);
    int find = 0;
    int child_inode_num;
    int i;
    for(i = 0; i < items; i++){
	if(strcmp(fip[i].file_name, filename) == 0){
	    child_inode_num = fip[i].ino;
	    find = 1;
	    break;
	}
    }

    if(!find){
	free(fip);
	fip = NULL;
	return -ENOENT;
    }

  // at first I wanted to find the block this entry is in and then remove this entry from it
  // but I reallized that if I did so, readdirfile may not will correctly if an entry is removed
  // because then dirsize in it might be negative, which could be a catastrophe
  // So now a just shrink the fip array and write it back to the directory blocks sequently
  // the last block will be restored if the size of the parent directory is multiple of 4096
    int j = i;
    if(j == items-1){
	*fip[j].file_name = '\0';
	fip[j].ino = -1;
    }
	
    for(j = i; j < items-1; j++){
	strcpy(fip[j].file_name, fip[j+1].file_name);
	fip[j].ino = fip[j+1].ino;
    }
    parent_inode->size -= sizeof(struct file_inode);
    inode_table[parent_inode_num].size -= sizeof(struct file_inode);

  // restore blocok
    if(inode_table[parent_inode_num].size % SECTOR_SIZE == 0){
	int blocks = parent_inode->size / SECTOR_SIZE;
	if(blocks < 4){
	    clear_bitmap_bit(inode_table[parent_inode_num].direct_pointer[blocks], dblock_bitmap);
	    inode_table[parent_inode_num].direct_pointer[blocks] = 0xffffffff;
	}
	else if(blocks == 4){
	    clear_bitmap_bit(inode_table[parent_inode_num].one_level_pointer, dblock_bitmap);
	    inode_table[parent_inode_num].one_level_pointer = 0xffffffff;
	}
	else{
	    unsigned int buffer[SECTOR_SIZE/4];
	    device_read_sector((unsigned char*)buffer, parent_inode->one_level_pointer);
	    clear_bitmap_bit(buffer[blocks-4], dblock_bitmap);
	    buffer[blocks-4] = 0xffffffff;
	    device_write_sector((unsigned char*)buffer, parent_inode->one_level_pointer);
	    device_flush();
	}
    }
    writedirfile(fip,
		 inode_table[parent_inode_num].size,
		 inode_table[parent_inode_num].direct_pointer,
		 inode_table[parent_inode_num].one_level_pointer);
    device_flush();
    free(fip);
    fip = NULL;
    return 0;
}

int p6fs_mkdir(const char *path, mode_t mode)
{
     /*do path parse here
      create dentry and update your index*/
    int parent_inode_num = pathref(path, PARENT);

    
    if(parent_inode_num == -ENOENT){
	return -ENOENT;
    }
    
    struct inode_t parent_inode;
    getinode(&parent_inode, parent_inode_num);

    // read the whole parent directory
    struct file_inode *fip;
    struct file_inode new_dir;
    // one extro file_inode
    fip = (struct file_inode*)malloc(parent_inode.size+(sizeof(struct file_inode)));
    if(!fip)
	return -ENOMEM;
    readdirfile(fip,
		parent_inode.size,
		parent_inode.direct_pointer,
		parent_inode.one_level_pointer);

    getfilename(path, new_dir.file_name);
    if(*new_dir.file_name == '\0'){
	free(fip);
	return -ENOBUFS;	
    }


    // check if there already exists a file with the same name
    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);

    if(allocate_and_create_dir(&parent_inode, parent_inode_num, fip, new_dir.file_name) == -1){
	free(fip);
	pthread_mutex_unlock(&bitmap_lock);
	pthread_mutex_unlock(&inode_lock);
	return -ENOENT;	
    }


    // update all metadata
    update_metadata(parent_inode_num == ROOT_INODE);

    device_flush();
    free(fip);
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    return 0;
}

int p6fs_rmdir(const char *path)
{
    /*
      - path parse->find the parent directory: root diretory can not be removed
      - get filename
      - remove entry of this file from parent directory
      - . and .. can not be removed, non-empty directory (size is 128) can not be removed
      - restore datablocks and inode of removed directory
      - update inode_bitmap and dblock_bimap
      - if the deleted entry is exactly the last entry of one block, restore this block
        set corresponding pointer to 0xffffffff(so I will not skip blocks)
     */
    int parent_inode_num, child_inode_num;
    struct inode_t parent_inode, child_inode;

    char filename[60];
    parent_inode_num = pathref(path, PARENT);
    child_inode_num = pathref(path, CHILD);
    if(parent_inode_num == -ENOENT || child_inode_num == -ENOENT)
	return -ENOENT;

    getinode(&parent_inode, parent_inode_num);
    getinode(&child_inode, child_inode_num);
    getfilename(path, filename);
    if(*filename == '\0')
	return -ENOENT;
    
    if(strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 || child_inode.size > 128)
	return -ENOTEMPTY;

    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);
    remove_entry(&parent_inode, parent_inode_num, filename);
    // now entry is removed, dblock bitmap is settled, restore inode
    clear_bitmap_bit(child_inode_num, inode_bitmap);

    // TODO restore dblock_bitmap bits
    update_metadata(parent_inode_num == ROOT_INODE);

    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    return 0;
}

int p6fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileInfo)
{
    int inode_num = pathref(path, CHILD);
    if(inode_num == -ENOENT)
	return -ENOENT;
    struct file_inode *files;
    struct inode_t inode;

    getinode(&inode, inode_num);
    
    files = (struct file_inode*)malloc(inode.size);
    if(!files)
	return -ENOMEM;
    readdirfile(files, inode.size, inode.direct_pointer, inode.one_level_pointer);

    if(inode.mode != DIR_T)
	return -ENOENT;
    else{
	for(unsigned long i = 0; i < inode.size / sizeof(struct file_inode); i++){
	    if(filler(buf, files[i].file_name, NULL, 0) == 1)
		break;
	}
    }
    return 0;    
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

    int inode_num = pathref(path, CHILD);

    if(inode_num == -ENOENT)
	return -ENOENT;

    struct inode_t inode;
    getinode(&inode, inode_num);

    int i, find = 0;
    for(i = 0; i < MAX_OPEN_FILE; i++){
	if(fd_table[i].allocated == 0){
	    find = 1;
	    break;
	}
    }

    if(!find){
	return -ENOENT;
    }

    fd_table[i].node = (struct inode_t*)malloc(sizeof(struct inode_t));
    fd_table[i].allocated = 1;
    fd_table[i].flag = fileInfo->flags;
    //assign and init your file handle
//    struct file_info *fi;

    if((fd_table[i].flag & 3) != O_RDONLY)
	return -EACCES;
    
    //check open flags, such as O_RDONLY
    //O_CREATE is tansformed to mknod() + open() by fuse ,so no need to create file here
    /*
     if(fileInfo->flags & O_RDONLY){
     fi->xxxx = xxxx;
     }
     */
    
    fileInfo->fh = (uint64_t)(fd_table+i);
    return 0;
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

    memset(statbuf, 0, sizeof(struct stat));
    
    int inode_num = pathref(path, CHILD);
    if(inode_num == -ENOENT)
	return -ENOENT;
    else{
	struct inode_t inode;
	getinode(&inode, inode_num);
	if(inode.mode == DIR_T){
	    statbuf->st_mode = S_IFDIR | 755;
	    statbuf->st_nlink = inode.nlinks;
	}
	else{
	    statbuf->st_mode = S_IFREG | 0444;
	    statbuf->st_nlink = 1;
	    statbuf->st_size = inode.size;
	}
    }
    return 0;
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





static int p6_mount(struct superblock_t *sb){
    // read superblock, two bitmap, inodes
    // initialize fd table
    // read root directory
    unsigned char buffer[SECTOR_SIZE];
    device_read_sector(buffer, SUPERBLOCK);
    memcpy(sb, buffer, sizeof(struct superblock_t));


    // initialize file descriptor table
    for(int i = 0; i < MAX_OPEN_FILE; i++){
	fd_table[i].fd = i;
	fd_table[i].flag = 0;
	fd_table[i].node = NULL;
	fd_table[i].allocated = 0;
    }

    rootdir.ino = 0;
    rootdir.dir_size = sb->root_dir_size;

    // read the whole directory in memory;
    // each block stores 64 entries;
    rootdir.fi = (struct file_inode*)malloc(rootdir.dir_size);  // will not be freed
    struct inode_t dirinode;
    getinode(&dirinode, ROOT_INODE);
    readdirfile(rootdir.fi, rootdir.dir_size, dirinode.direct_pointer, dirinode.one_level_pointer);

    // read superblock
    // device_read_sector(buffer, SUPERBLOCK);
    // memcpy(sb, buffer, sizeof(struct superblock_t));

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

    pthread_mutex_init(&bitmap_lock, NULL);
    pthread_mutex_init(&inode_lock, NULL);
}

static int p6_mkfs(struct superblock_t *sb){
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
    inode_table[ROOT_INODE].mode = DIR_T;
    inode_table[ROOT_INODE].nlinks = 2;
    inode_table[ROOT_INODE].size = sb->root_dir_size;
    for(int i = 0; i < 4; i++){
	inode_table[ROOT_INODE].direct_pointer[i] = i + ROOT_DIR;
    }
    inode_table[ROOT_INODE].one_level_pointer = 4 + ROOT_DIR;

    // Now we may deal with the bitmaps
    // inode bitmap
    // this is initializing stage, so of one inode is used: rootdir
    inode_bitmap[0] |= group[GI1];

    // data block bimap
    dblock_bitmap[0] |= ~(char)0x07;     // former 5 blocks are reserved for root dir

    // write superblock
    device_write_sector((unsigned char*)sb, SUPERBLOCK);  

    // write inode bitmap
    unsigned int i;
    for(i = 0; i < INODE_BITMAP_BLOCKS; i++){
	device_write_sector((unsigned char*)inode_bitmap+i*SECTOR_SIZE,
			    sb->inode_bitmap_base+i);
    }

    // write data block bitmap
    for(i = 0; i < DBLOCK_BITMAP_BLOCKS; i++){
	device_write_sector((unsigned char*)dblock_bitmap+i*SECTOR_SIZE,
			    sb->dblock_bitmap_base+i);
    }

    // wrtie inodes
    unsigned char* cp_inode = (unsigned char*)inode_table;
    for(i = 0; i < INODE_BLOCKS; i++){
	device_write_sector(cp_inode + i*SECTOR_SIZE, sb->inode_table_base+i);
    }

    // write spare superblock
    device_write_sector((unsigned char*)sb, SPSUPERBLOCK);

    // write directory entry . and .. to rootdir
    struct file_inode entries[2] = {
	{.file_name = ".", .ino = 0},
	{.file_name = "..", .ino = 0},
    };
    device_write_sector((unsigned char*)entries, ROOT_DIR);
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
    char *diskpath = "/home/frostfall/Desktop/OSLabFile/Github/P6/Project6-File-System/project6-start-code/DISK";
    if(device_open(diskpath) != 0){
	printf("Invalid disk\n");
	exit(-1);
    }

    device_read_sector(buffer, SUPERBLOCK);
    memcpy(&glo_superblock, buffer, sizeof(struct superblock_t));

    device_read_sector(buffer, SPSUPERBLOCK);
    memcpy(&ssp, buffer, sizeof(struct superblock_t));
    if(glo_superblock.magic != 1){
	if(ssp.magic != 1)     // superblock and spare superblock are both broken, mkfs
	    p6_mkfs(&glo_superblock);
	else{                  // recover data from spare superblock
	    memcpy(buffer, &ssp, sizeof(struct superblock_t));
	    device_write_sector(buffer, SUPERBLOCK);
	    device_flush();
	}
    }

    p6_mount(&glo_superblock);
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
