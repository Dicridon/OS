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
  65       66       67       16448

  |             |
  | super block |
  |             |
  16449

  |  root  |  root  |  root  |  root  |  root  |        |        |
  | dblock | dblock | dblock | dblock | dblock | ...... | dblock |
  |        |        |        |        |        |        |        |
  16450    16451    16452    16453    16454    16455

 **************************************************************************
 **************************************************************************/


struct inode_t inode_table[INODE_NUM];          // 8192 blocks in total
unsigned char inode_bitmap[GROUP_NUM];        // 32 blocks in total
unsigned char dblock_bitmap[GROUP_NUM];       // 32 blocks in total
struct dentry rootdir;
struct dentry workdir;
struct superblock_t glo_superblock;
struct file_info fd_table[MAX_OPEN_FILE] ;
struct file_inode name_hash[MAX_OPEN_FILE];

pthread_mutex_t bitmap_lock;  // lock both inode_bitmap and dblock_bitmap
pthread_mutex_t inode_lock;


// DO allocate sapce for char *name
static void getfilename(const char* path, char* filename){
    char* name = path + strlen(path);
    if(*(name-1) == '/'){
	*filename = '/';
	return;
    }

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


// this hash is just a cache like stuff, so conflicting will causes substitution
static int str2index(const char *name){
    char *walk = name;
    int seed = 131;
    unsigned int hash = 0;
    while(*walk){
	hash = hash * seed + (*walk);
	walk++;
    }
    return hash % MAX_OPEN_FILE;
}


// please allocate some memory to fip before calling function readdirfile
// this function will read whole dirctory into fip
// so specify directory size dirsize(bytes)
// pass the direct pointers and one-level pointer to base and ref

static void readdirfile(struct file_inode* fip, unsigned int dirsize, unsigned int* base, unsigned int ref){
    unsigned int blocks = dirsize / SECTOR_SIZE;
    blocks += (dirsize % SECTOR_SIZE != 0);
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
    blocks += (dirsize % SECTOR_SIZE != 0);
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

static int extend_dir(int parent_inode_num){
    int block = inode_table[parent_inode_num].size / SECTOR_SIZE;
    int free_block = bitmap_lookup(dblock_bitmap);
    if(free_block == -1)
	return -ENOMEM;
    write_bitmap_bit(free_block, dblock_bitmap);
    if(block < 4){
	inode_table[parent_inode_num].direct_pointer[block] = free_block + DBLOCK_BASE;
    }
    else if(block == 4){
	int free_block2 = bitmap_lookup(dblock_bitmap);
	unsigned int pointers[SECTOR_SIZE/4];
	inode_table[parent_inode_num].one_level_pointer = free_block + DBLOCK_BASE;
	write_bitmap_bit(free_block2, dblock_bitmap);
	memset(pointers, 0, SECTOR_SIZE);
	pointers[block-4] = free_block2;
	device_write_sector((unsigned char*)pointers, inode_table[parent_inode_num].one_level_pointer);
    }
    else{
	unsigned int pointers[SECTOR_SIZE/4];
	device_read_sector((unsigned char*)pointers, inode_table[parent_inode_num].one_level_pointer);
	pointers[block-4] = free_block + DBLOCK_BASE;
	device_write_sector((unsigned char*)pointers, inode_table[parent_inode_num].one_level_pointer);
    }
    return 0;
}

// this function will allocate one inode form the filename, so it should not be used in mknod
// but it will not extend parent directory
static int allocate_and_create_entry(int parent_inode_num, struct file_inode *fip, char* filename, node_type type, const char* link_to){
    int items = inode_table[parent_inode_num].size / sizeof(struct file_inode);
    int pindex;
    for(pindex = 0; pindex < items; pindex++){
	if(strcmp(filename, fip[pindex].file_name) == 0)
	    return -EEXIST;      // directory or file already exsits
    }

    // first deal with new inode
    int empty_inode_num = bitmap_lookup(inode_bitmap);
    if(empty_inode_num == -1)
	return -ENOMEM;

    int slot;
    slot = bitmap_lookup(dblock_bitmap);
    if(slot == -1)
	return -ENOMEM;

    for(int i = 0; i < 4; i++)
	inode_table[empty_inode_num].direct_pointer[i] = 0xffffffff;

    write_bitmap_bit(empty_inode_num, inode_bitmap);
    inode_table[empty_inode_num].mode = type;
    inode_table[empty_inode_num].nlinks = (type == DIR_T) ? 2 : 1;
    inode_table[empty_inode_num].size = (type == DIR_T) ? 2 * sizeof(struct file_inode) : 0;    
    inode_table[empty_inode_num].direct_pointer[0] = slot + DBLOCK_BASE;
    inode_table[empty_inode_num].one_level_pointer = 0xffffffff;
    inode_table[empty_inode_num].two_level_pointer = 0xffffffff;
    inode_table[empty_inode_num].create_time = time(NULL);
    inode_table[empty_inode_num].last_access_time = inode_table[empty_inode_num].create_time;
    inode_table[empty_inode_num].last_modified_time = inode_table[empty_inode_num].last_access_time;
    inode_table[empty_inode_num].access_mod = (type == DIR_T || type == SYMLINK_T) ? 0755 : 0644;

    // update parent directory
    strcpy(fip[pindex].file_name, filename);
    fip[pindex].ino = empty_inode_num;
    inode_table[parent_inode_num].size += sizeof(struct file_inode);
    writedirfile(fip,
		 inode_table[parent_inode_num].size,
		 inode_table[parent_inode_num].direct_pointer,
		 inode_table[parent_inode_num].one_level_pointer);
    inode_table[parent_inode_num].last_access_time = inode_table[empty_inode_num].last_access_time;
    inode_table[parent_inode_num].last_modified_time = inode_table[empty_inode_num].last_access_time;
    
    // write child directory
    if(type == DIR_T){
	write_bitmap_bit(slot, dblock_bitmap);
	struct file_inode new_fip[2] =  {
	    {.file_name = ".", .ino = empty_inode_num},
	    {.file_name = "..", .ino = parent_inode_num}
	};
	writedirfile(new_fip,
		     inode_table[empty_inode_num].size,
		     inode_table[empty_inode_num].direct_pointer,
		     inode_table[empty_inode_num].one_level_pointer);
    }
    else if(type == SYMLINK_T){
	unsigned char buffer[SECTOR_SIZE];
	write_bitmap_bit(slot, dblock_bitmap);
	if(strlen(link_to) >= SECTOR_SIZE)
	    return -ENOBUFS;
	memcpy(buffer, link_to, strlen(link_to));
	device_write_sector(buffer, inode_table[empty_inode_num].direct_pointer[0]);
    }
    else{
	inode_table[empty_inode_num].direct_pointer[0] = 0xffffffff;
    }
    
    device_flush();
    glo_superblock.available_blocks--;
    glo_superblock.available_inodes--;
    return 0;
}


// notice this function is only used to add an entry to a directory setting ino to inode_num
// so call this function to add an entry in p6fs_link
static int create_entry(struct inode_t *parent_inode, int parent_inode_num, char* filename, int inode_num){
    struct file_inode *fip;
    parent_inode->size = inode_table[parent_inode_num].size;
    fip = (struct file_inode*)malloc(parent_inode->size + sizeof(struct file_inode));
    if(fip == NULL){
	return -ENOMEM;
    }

    readdirfile(fip, parent_inode->size,
		parent_inode->direct_pointer,
		parent_inode->one_level_pointer);

    int items = parent_inode->size / sizeof(struct file_inode);
    int pindex;
    for(pindex = 0; pindex < items; pindex++){
	if(strcmp(filename, fip[pindex].file_name) == 0){
	    free(fip);
	    return -EEXIST;      // directory or file already exsits
	}

    }
    // update parent directory
    strcpy(fip[pindex].file_name, filename);
    fip[pindex].ino = inode_num;
    parent_inode->size += sizeof(struct file_inode);
    writedirfile(fip,
		 parent_inode->size,
		 parent_inode->direct_pointer,
		 parent_inode->one_level_pointer);

    // update inode_table
    inode_table[parent_inode_num].size = parent_inode->size;
    device_flush();
    free(fip);
    return 0;
}


static void nullptrcheck(void* p){
    if(p == NULL){
	DEBUG("NULL POINTRE %s, %d", __FILE__, __LINE__);
	exit(-1);
    }
}

static int pathref(const char* path, int parent){
    int name_index;
    int hashed = 1;
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
    currdir = &rootdir;
    while(token != NULL){
	name_index = str2index(token);
	items = currdir->dir_size / sizeof(struct file_inode);
	for(i = 0; i < items; i++){
	    if(strcmp(name_hash[name_index].file_name, token) == 0){
		inode_num = name_hash[name_index].ino;
		find = 1;
		break;
	    }
	    hashed = 0;
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
	// update hash table
	strcpy(name_hash[name_index].file_name, token);
	name_hash[name_index].ino = inode_num;
	hashed = 1;
	
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

    free(innerpath);
    free(tempdir.fi);
    return inode_num;
}




static int remove_entry(struct inode_t *parent_inode, int parent_inode_num, const char* filename){
    struct file_inode *fip = (struct file_inode*)malloc(parent_inode->size);
    if(fip == NULL)
	return -ENOMEM;
    memset(fip, 0, parent_inode->size);
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

    int j = i;
    if(j == items-1){
	memset(fip[j].file_name, 0, strlen(fip[j].file_name));
	fip[j].ino = -1;
    }
	
    for(j = i; j < items-1; j++){
	strcpy(fip[j].file_name, fip[j+1].file_name);
	fip[j].ino = fip[j+1].ino;
    }
    inode_table[parent_inode_num].size -= sizeof(struct file_inode);
    parent_inode->size -= sizeof(struct file_inode);
    writedirfile(fip,
		 inode_table[parent_inode_num].size,
		 inode_table[parent_inode_num].direct_pointer,
		 inode_table[parent_inode_num].one_level_pointer);
    free(fip);
    device_flush();
    return 0;
}

// at first I wanted to find the block this entry is in and then remove this entry from it
// but I reallized that if I did so, readdirfile may not will correctly if an entry is removed
// because then dirsize in it might be negative, which could be a catastrophe
// So now a just shrink the fip array and write it back to the directory blocks sequently
// the last block will be restored if the size of the parent directory is multiple of 4096
static int remove_entry_and_restore(struct inode_t *parent_inode, int parent_inode_num, const char* filename){
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
    unsigned int buffer[SECTOR_SIZE/4];
    if(inode_table[parent_inode_num].size % SECTOR_SIZE == 0){
	int blocks = parent_inode->size / SECTOR_SIZE;
	if(blocks < 4){
	    clear_bitmap_bit(inode_table[parent_inode_num].direct_pointer[blocks], dblock_bitmap);
	    inode_table[parent_inode_num].direct_pointer[blocks] = 0xffffffff;
	}
	else if(blocks == 4){
	    device_read_sector((unsigned char*)buffer, parent_inode->one_level_pointer);
	    clear_bitmap_bit(buffer[blocks-4], dblock_bitmap);
	    clear_bitmap_bit(inode_table[parent_inode_num].one_level_pointer, dblock_bitmap);
	    inode_table[parent_inode_num].one_level_pointer = 0xffffffff;
	}
	else{

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
    glo_superblock.available_blocks++;
    glo_superblock.available_inodes++;
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
    if(parent_inode.mode != DIR_T)
	return -ENOTDIR;
    else if(parent_inode.size > 4 * M)
	return -ENOMEM;
    // read the whole parent directory
    struct file_inode *fip;
    struct file_inode new_dir;
    // one extro file_inode
    if(parent_inode.size % SECTOR_SIZE == 0)
	if(extend_dir(parent_inode_num) != 0)
	    return -ENOMEM;
    fip = (struct file_inode*)malloc(parent_inode.size+(sizeof(struct file_inode)));
    if(!fip)
	return -ENOMEM;
    readdirfile(fip,
		parent_inode.size,
		parent_inode.direct_pointer,
		parent_inode.one_level_pointer);

    getfilename(path, new_dir.file_name);
    if(*new_dir.file_name == '\0' || strlen(new_dir.file_name) >= FILENAME_MAX){
	free(fip);
	return -ENOBUFS;	
    }


    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);

    if(allocate_and_create_entry(parent_inode_num, fip, new_dir.file_name, DIR_T, "") != 0){
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
	return -ENOBUFS;
    
    if(strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0 || child_inode.size > 128)
	return -ENOTEMPTY;

    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);
    remove_entry(&parent_inode, parent_inode_num, filename);
    // now entry is removed, dblock bitmap is settled, restore inode
    clear_bitmap_bit(child_inode_num, inode_bitmap);
    clear_bitmap_bit(child_inode.direct_pointer[0]-DBLOCK_BASE, dblock_bitmap);
    update_metadata(parent_inode_num == ROOT_INODE);
    device_flush();
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

    if(inode.mode != DIR_T){
	free(files);
	return -ENOTDIR;
    }
    else{
	for(unsigned long i = 0; i < inode.size / sizeof(struct file_inode); i++){
	    if(filler(buf, files[i].file_name, NULL, 0) == 1){
		free(files);
		return -ENOBUFS;		
	    }
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
    int parent_inode_num = pathref(path, PARENT);
    
    if(parent_inode_num == -ENOENT){
	return -ENOENT;
    }
    
    struct inode_t parent_inode;
    getinode(&parent_inode, parent_inode_num);
    if(parent_inode.mode != DIR_T)
	return -ENOTDIR;
    else if(parent_inode.size > 4*M)
	return -ENOMEM;
    // read the whole parent directory
    struct file_inode *fip;
    struct file_inode new_dir;
    // one extro file_inode
    if(parent_inode.size % SECTOR_SIZE == 0)
	if(extend_dir(parent_inode_num) != 0)
	    return ENOMEM;
    fip = (struct file_inode*)malloc(parent_inode.size+(sizeof(struct file_inode)));
    if(!fip)
	return -ENOMEM;
    readdirfile(fip,
		parent_inode.size,
		parent_inode.direct_pointer,
		parent_inode.one_level_pointer);

    getfilename(path, new_dir.file_name);
    if(*new_dir.file_name == '\0' || strlen(new_dir.file_name) >= FILENAME_MAX){
	free(fip);
	return -ENOBUFS;	
    }

    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);

    if(allocate_and_create_entry(parent_inode_num, fip, new_dir.file_name, FILE_T, "") != 0){
	free(fip);
	pthread_mutex_unlock(&bitmap_lock);
	pthread_mutex_unlock(&inode_lock);
	return -ENOMEM;	
    }

    // update all metadata
    update_metadata(parent_inode_num == ROOT_INODE);
    
    device_flush();
    free(fip);
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    return 0;
}

int p6fs_readlink(const char *path, char *link, size_t size){
    int inode_num = pathref(path, CHILD);
    if(inode_num == -ENOENT)
	return -ENOENT;

    unsigned char buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);
    device_read_sector(buffer, inode_table[inode_num].direct_pointer[0]);
    memcpy(link, buffer, (size > SECTOR_SIZE) ? SECTOR_SIZE : size);
    return 0;
}

    


int p6fs_symlink(const char *path, const char *link)
{
    if(strlen(link) >= SECTOR_SIZE)
	return -ENOBUFS;
	
    int parent_inode_num = pathref(link, PARENT);
    char filename[FILENAME_MAX] = "";
    getfilename(link, filename);

    if(*filename == '\0')
	return -ENOBUFS;
    
    struct inode_t parent_inode;
    getinode(&parent_inode, parent_inode_num);
    

    struct file_inode *fip = (struct file_inode*)malloc(sizeof(struct file_inode) + parent_inode.size);
    if(fip == NULL){
	return -ENOMEM;
    }

    readdirfile(fip,
		parent_inode.size,
		parent_inode.direct_pointer,
		parent_inode.one_level_pointer);
    
    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);
    if(allocate_and_create_entry(parent_inode_num, fip, filename, SYMLINK_T, path) != 0){
	pthread_mutex_unlock(&bitmap_lock);
	pthread_mutex_unlock(&inode_lock);
	free(fip);
	return -ENOMEM;
    }

    update_metadata(parent_inode_num == ROOT_INODE);
    device_flush();
    free(fip);
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    return 0;
}

int p6fs_link(const char *path, const char *newpath)
{

    if(strcmp(path, newpath) == 0)
	return -EEXIST;
    
    char new_filename[FILENAME_MAX];
    char old_filename[FILENAME_MAX];
    getfilename(newpath, new_filename);
    getfilename(path, old_filename);

    if(*new_filename == '\0' || *old_filename == '\0' )
	return -ENOBUFS;

//    int old_parent_inode_num = pathref(path, PARENT);
    int new_parent_inode_num = pathref(newpath, PARENT);
//    if(old_parent_inode_num == new_parent_inode_num && (strcmp(new_filename, old_filename) == 0))
//	return -EEXIST;
    
    struct inode_t new_parent_inode;
    int child_inode_num = pathref(path, CHILD);
    getinode(&new_parent_inode, new_parent_inode_num);
    
    if(new_parent_inode_num == -ENOENT || child_inode_num == -ENOENT)
	return -ENOENT;
    if(new_parent_inode.size > 4*M)
	return -ENOMEM;
    
    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);
    
    if(new_parent_inode.size % SECTOR_SIZE == 0)
	if(extend_dir(new_parent_inode_num) == -1)
	    return -ENOMEM;
    if(create_entry(&new_parent_inode, new_parent_inode_num, new_filename, child_inode_num) != 0){
	pthread_mutex_unlock(&bitmap_lock);
	pthread_mutex_unlock(&inode_lock);
	return -EEXIST;	
    }


    inode_table[child_inode_num].nlinks++;
    update_metadata(new_parent_inode_num == ROOT_INODE);
    device_flush();
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    return 0;
}

static void restore_file_dblocks(struct inode_t *inode){
    
    int blocks = inode->size / SECTOR_SIZE;
    if(blocks == 0){
	clear_bitmap_bit(inode->direct_pointer[0]-DBLOCK_BASE, dblock_bitmap);
	return;
    }
    blocks += (inode->size % SECTOR_SIZE != 0);
    int limit = (blocks <= 4) ? blocks : 4;
    for(int i = 0; i < limit; i++){
	clear_bitmap_bit(inode->direct_pointer[i]-DBLOCK_BASE, dblock_bitmap);
    }

    if(blocks <= 4)
	return;

    blocks -= 4;
    unsigned int buffer[SECTOR_SIZE/4];
    device_read_sector((unsigned char*)buffer, inode->one_level_pointer);
    for(int i = 0; i < blocks; i++){
	clear_bitmap_bit(buffer[i]-DBLOCK_BASE, dblock_bitmap);
    }
}

int p6fs_unlink(const char *path)
{
    int child_inode_num;
    child_inode_num = pathref(path, CHILD);
    if(child_inode_num == -ENOENT)
	return -ENOENT;

    int parent_inode_num;
    struct inode_t parent_inode;
    parent_inode_num = pathref(path, PARENT);
    if(parent_inode_num == -ENOENT)
	return -ENOENT;
    
    getinode(&parent_inode, parent_inode_num);    
    struct inode_t child_inode;
    getinode(&child_inode, child_inode_num);

    char filename[FILENAME_MAX];
    getfilename(path, filename);
    if(*filename == '\0')
	return -ENOBUFS;
    int status;
    if(child_inode.mode == DIR_T)
	return -ENOTDIR;
    else if(--inode_table[child_inode_num].nlinks > 0){
	status = remove_entry(&parent_inode, parent_inode_num, filename);
	update_metadata(parent_inode_num == ROOT_INODE);
	return status;
    }

    // restore blocks and remove entry in parent directory
    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);

    // remove entry in parent directory
    status = remove_entry_and_restore(&parent_inode, parent_inode_num, filename);
    // resotre inode
    clear_bitmap_bit(child_inode_num, inode_bitmap);
    // resotre data blocks
    restore_file_dblocks(&child_inode);
    update_metadata(parent_inode_num == ROOT_INODE);
    device_flush();
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    
    return 0;
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
	return -EMFILE;
    }

    fd_table[i].node = &inode_table[inode_num];
    fd_table[i].allocated = 1;
    fd_table[i].flag = fileInfo->flags;
    fd_table[i].inode_num = inode_num;
    
    //assign and init your file handle
//    struct file_info *fi;

//    if((fd_table[i].flag & 3) != O_RDONLY)
//	return -EACCES;
    //check open flags, such as O_RDONLY
    //O_CREATE is tansformed to mknod() + open() by fuse ,so no need to create file here
    /*
     if(fileInfo->flags & O_RDONLY){
     fi->xxxx = xxxx;
     }
     */
    inode_table[inode_num].last_access_time = time(NULL);
    fileInfo->fh = (uint64_t)(fd_table+i);
    return 0;
}

// below are functions only for read, wirte and truncate

static void read_file_pointers(unsigned int *pointers, int start_block, int total_blocks, int inode_num){
    unsigned int *direct_pointers = inode_table[inode_num].direct_pointer;
    unsigned int one_level_pointers[SECTOR_SIZE/4];
    unsigned int pointers_to_two_level_pointers[SECTOR_SIZE/4];
    if(inode_table[inode_num].one_level_pointer != 0xffffffff){
	device_read_sector((unsigned char*)one_level_pointers,
			   inode_table[inode_num].one_level_pointer);
    }
    if(inode_table[inode_num].two_level_pointer != 0xffffffff){
	device_read_sector((unsigned char*)pointers_to_two_level_pointers,
			   inode_table[inode_num].two_level_pointer);
    }

    // first copy direct pointers
    int index_in_pointers = 0;
    if(start_block < 4){
	for(int i = 0; i+start_block < 4 && total_blocks > 0; i++){
	    pointers[index_in_pointers++] = direct_pointers[i+start_block];
	    total_blocks--;
	}
    }

    // copy one-level-pointers
    start_block -= 4;   // if start block is in one-level-indexing blocks
    if(total_blocks > 0){
	for(int i = 0; i + start_block < 1024 && total_blocks > 0; i++){
	    pointers[index_in_pointers++] = one_level_pointers[i+start_block];
	    total_blocks--;
	}
    }

    // copy two-level-pointers
    start_block -= 1024;
    int firsttime = 1; // pretend that all the two-level-pointers are in a single array
    if(total_blocks > 0){
	unsigned int two_level_pointers[SECTOR_SIZE/4];
	for(int j = start_block / 1024; j < 1024 && total_blocks > 0; j++){
	    device_read_sector((unsigned char *)two_level_pointers,
			       pointers_to_two_level_pointers[j]);
	    for(int i = (firsttime) ? start_block % 1024 : 0; i < 1024 && total_blocks > 0; i++){
		pointers[index_in_pointers++] = two_level_pointers[i];
		total_blocks--;
	    }
	    firsttime = 0;     // must set it to zero
	}
    }
}

static int readregfile(char *buf, unsigned int block_offset, long size, unsigned int *pointers, int blocks){
    unsigned char buffer[SECTOR_SIZE];
    int index = 0;
    unsigned int bytes = 0;
    // read first block
    device_read_sector(buffer, pointers[index++]);
    memcpy(buf,
	   buffer+block_offset,
	   (SECTOR_SIZE - block_offset> size) ? size : SECTOR_SIZE-block_offset);
    if(size <= SECTOR_SIZE-block_offset)
	return size;

    // read rest blocks
    bytes += SECTOR_SIZE - block_offset;
    size -= SECTOR_SIZE - block_offset;
    for(; index < blocks; index++){
	device_read_sector(buffer, pointers[index]);
	int copy_size = (SECTOR_SIZE > size) ? size : SECTOR_SIZE;
	memcpy(buf+ block_offset % SECTOR_SIZE + (index-1)*SECTOR_SIZE, buffer, copy_size);
	bytes += copy_size;
	size -= copy_size;
    }
    return bytes;
}

// test: cat a symbol link to see if the path in that link can be read correctly
int p6fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    /* get inode from file handle and do operation*/
    struct file_info *fd = (struct file_info *)fileInfo->fh;
    int readbytes = 0;
    int start_block;
    int start_point_in_start_block;
    int end_block;
    int end_point_in_end_block;
    int total_blocks;
    int file_blocks;    // blocks a file really occupies
    int available_blocks;
    unsigned int *block_pointers = NULL;

    if((fileInfo->flags & O_WRONLY) == O_WRONLY)
	return -EACCES;
    
    if(!fd->allocated)
	return -ENOMEM;
    /*
      At first I wanted to read the context of a file into memory and then copy those bytes to buf
      but this involved too many boundary conditions. So I changed my strategy: read all the
      pointers to blocks into memory, which will not be too large, (4 + 1024 + 1024)*4 bytes,
      then compute which blocks will be used according to size and offset, then we just read those 
      blocks using device_read.
     */
    if(offset > fd->node->size || fd->node->size == 0){
	memset(buf, 0, size);
	return 0;
    }
	
    start_block = offset / SECTOR_SIZE;
    start_point_in_start_block = offset % SECTOR_SIZE;
    end_block = (size - 1 + offset) / SECTOR_SIZE;
    end_point_in_end_block = (size - 1 + offset) % SECTOR_SIZE;

    file_blocks = (fd->node->size / SECTOR_SIZE) + (fd->node->size % SECTOR_SIZE != 0);
    total_blocks = end_block - start_block + 1;
    available_blocks = file_blocks - start_block + 1;
    total_blocks = (available_blocks <= total_blocks) ? available_blocks : total_blocks;
    
    block_pointers = (unsigned int *)malloc(total_blocks);

    // code above limit block access with blocks a file really have strctly
    // so below may read all the needed pointers sequently safely
    read_file_pointers(block_pointers, start_block, total_blocks, fd->inode_num);

    // now we may read context of a file
    size = (size >= fd->node->size) ? fd->node->size : size;
    readbytes = readregfile(buf, offset/SECTOR_SIZE, size, block_pointers, total_blocks);
    
    pthread_mutex_lock(&inode_lock);
    fd->node->last_access_time = time(NULL);
    pthread_mutex_unlock(&inode_lock);
    
    free(block_pointers);
    return readbytes;
}

static int writeregfile(const char *buf, long size, off_t offset, unsigned int *pointers, int blocks){
    unsigned char buffer[SECTOR_SIZE];
    memset(buffer, 0, SECTOR_SIZE);
    int bytes = 0;
    int index = 0;
    int copysize;
    device_read_sector(buffer, pointers[0]);
    copysize = (SECTOR_SIZE - offset - 1 > size) ? size : SECTOR_SIZE-offset-1;
    bytes += copysize;
    memcpy(buffer+(offset % SECTOR_SIZE),
	   buf,
	   (SECTOR_SIZE-offset) > size ? size : SECTOR_SIZE-offset);
    device_write_sector(buffer, pointers[index++]);
    for(; index < blocks; index++){
	copysize = (SECTOR_SIZE > size) ? size : SECTOR_SIZE;
	memcpy(buffer, buf+offset%SECTOR_SIZE + (index-1)*SECTOR_SIZE, copysize);
	device_write_sector(buffer, pointers[index]);
	memset(buffer, 0, SECTOR_SIZE);	
	bytes += copysize;
	size -= copysize;
    }
    device_flush();
    return bytes;
}

// this function should only be called when it is sure that blocks owned are strictly more
// then blocks needed
static void shrink(int inode_num, int blocks_needed){
    int filesize = inode_table[inode_num].size;
    int blocks_owned = filesize / SECTOR_SIZE + (filesize % SECTOR_SIZE != 0);

    unsigned int one_level_pointers[SECTOR_SIZE/4];
    unsigned int pointers_to_two_level_pointers[SECTOR_SIZE/4];

    // direct pointers
    int redundant_blocks = blocks_owned - blocks_needed;
    if(blocks_needed < 4){
	for(int i = blocks_needed; i < blocks_owned && i < 4; i++){
	    clear_bitmap_bit(inode_table[inode_num].direct_pointer[i], dblock_bitmap);
	    redundant_blocks--;
	}
    }

    // one-level-pointers
    if(blocks_needed < 1028 && redundant_blocks > 0){
	device_read_sector((unsigned char *)one_level_pointers,
			   inode_table[inode_num].one_level_pointer);
	for(int i = blocks_needed-4; i < blocks_owned-4 && i < 1024; i++){
	    clear_bitmap_bit(one_level_pointers[i], dblock_bitmap);
	    redundant_blocks--;
	}
	if(blocks_needed == 4)
	    clear_bitmap_bit(inode_table[inode_num].one_level_pointer,
			     dblock_bitmap);
    }

    // two-level-pointers
    int firsttime = 1;
    if(redundant_blocks > 0){
	unsigned int buffer[SECTOR_SIZE/4];
	device_read_sector((unsigned char *)pointers_to_two_level_pointers,
			   inode_table[inode_num].two_level_pointer);
	for(int j = (blocks_needed-1028) / 1024; redundant_blocks > 0; j++){
	    for(int i = (firsttime) ? (blocks_needed-1028) % 1024 : 0; i < 1024 && redundant_blocks > 0; i++){
		device_read_sector((unsigned char *)buffer, pointers_to_two_level_pointers[j]);
		clear_bitmap_bit(buffer[i], dblock_bitmap);
		redundant_blocks--;
	    }
	    device_write_sector((unsigned char *)buffer, pointers_to_two_level_pointers[j]);
	    firsttime = 0;
	}
	if(blocks_needed == 1028)
	    clear_bitmap_bit(inode_table[inode_num].two_level_pointer, dblock_bitmap);
    }
    device_flush();
}

static void extend_file(int inode_num, int blocks_needed){
    int filesize = inode_table[inode_num].size;
    int blocks_owned = filesize / SECTOR_SIZE + (filesize % SECTOR_SIZE != 0);
    int slot;
    unsigned int one_level_pointers[SECTOR_SIZE/4];
    unsigned int pointers_to_two_level_pointers[SECTOR_SIZE/4];

    // how many extra blocks are needed
    blocks_needed -= blocks_owned;
    
    // direct pointers
    if(blocks_owned < 4){
	for(int i = blocks_owned; i < 4 && blocks_needed > 0; i++){
	    slot = bitmap_lookup(dblock_bitmap) + DBLOCK_BASE;
	    inode_table[inode_num].direct_pointer[i] = slot;
	    write_bitmap_bit(slot, dblock_bitmap);
	    blocks_needed--;
	}
    }

    // one-level-pointers
    if(blocks_needed > 0 && blocks_owned < 1028){
	if(blocks_owned == 4){
	    slot = bitmap_lookup(dblock_bitmap) + DBLOCK_BASE;
	    inode_table[inode_num].one_level_pointer = slot;
	    write_bitmap_bit(slot, dblock_bitmap);
	}
	device_read_sector((unsigned char *)one_level_pointers,
			   inode_table[inode_num].one_level_pointer);
	for(int i = blocks_owned-4; i < 1024 && blocks_needed > 0; i++){
	    slot = bitmap_lookup(dblock_bitmap) + DBLOCK_BASE;
	    one_level_pointers[i] = slot;
	    write_bitmap_bit(slot, dblock_bitmap);
	    blocks_needed--;
	}
	device_write_sector((unsigned char *)one_level_pointers,
			    inode_table[inode_num].one_level_pointer);
    }

    // two-level-pointers
    int firsttime = 1;
    if(blocks_needed > 0){
	unsigned int buffer[SECTOR_SIZE/4];
	if(blocks_owned == 1028){
	    slot = bitmap_lookup(dblock_bitmap) + DBLOCK_BASE;
	    inode_table[inode_num].two_level_pointer = slot;
	    write_bitmap_bit(slot, dblock_bitmap);
	}
	device_read_sector((unsigned char *)pointers_to_two_level_pointers,
			   inode_table[inode_num].two_level_pointer);

	for(int j = (blocks_owned - 1028) / 1024; j < 1024 && blocks_needed > 0; j++){
	    slot = bitmap_lookup(dblock_bitmap) + DBLOCK_BASE;
	    pointers_to_two_level_pointers[j] = slot;
	    write_bitmap_bit(slot, dblock_bitmap);
	    for(int i = (firsttime) ? (blocks_owned - 1028) % 1024 : 0; i < 1024 && blocks_needed > 0; i++){
		device_read_sector((unsigned char *)buffer,
				   pointers_to_two_level_pointers[j]);
		slot = bitmap_lookup(dblock_bitmap) + DBLOCK_BASE;
		buffer[i] = slot;
		write_bitmap_bit(slot, dblock_bitmap);
		blocks_needed--;
	    }
	    device_write_sector((unsigned char *)buffer, pointers_to_two_level_pointers[j]);
	    firsttime = 0;
	}
    }
    device_flush();
}

/* get inode from file handle and do operation*/
// seems that the offset will always be zero, so things get easy
/*
  if size > file->size
      extend_file
      get_pointers
      write_file
  else
      write_file
      shrink_file
      restore_blocks
*/
int p6fs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fileInfo)
{
    if((fileInfo->flags & 0x3) == O_RDONLY)
	return -EACCES;
    
    struct file_info *fd = (struct file_info *)fileInfo->fh;
    int blocks_owned = fd->node->size / SECTOR_SIZE + (fd->node->size % SECTOR_SIZE != 0);
    int blocks_needed = (offset + size) / SECTOR_SIZE + ((size + offset) % SECTOR_SIZE != 0);
    int bytes;
    int start_block;
    int start_point_in_start_block;
    int end_block;
    int end_point_in_end_block;
    int total_blocks;
    int file_blocks;    // blocks a file really occupies
    unsigned int *block_pointers = NULL;
    if(!fd->allocated)
	return -ENOMEM;
    start_block = offset / SECTOR_SIZE;
    start_point_in_start_block = offset % SECTOR_SIZE;
    end_block = (size - 1 + offset) / SECTOR_SIZE;
    end_point_in_end_block = (size - 1 + offset) % SECTOR_SIZE;

    file_blocks = (fd->node->size / SECTOR_SIZE) + (fd->node->size % SECTOR_SIZE != 0);
    total_blocks = end_block - start_block + 1;
    
    block_pointers = (unsigned int *)malloc(total_blocks);
    if(block_pointers == NULL){
	return -ENOMEM;
    }
	
    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);
    
    if(blocks_needed == blocks_owned){
	read_file_pointers(block_pointers, start_block, total_blocks, fd->inode_num);
    }
    else if(blocks_needed < blocks_owned){
	read_file_pointers(block_pointers, start_block, total_blocks, fd->inode_num);
	shrink(fd->inode_num, blocks_needed);
    }
    else{
	extend_file(fd->inode_num, blocks_needed);
	read_file_pointers(block_pointers, start_block, total_blocks, fd->inode_num);
    }
    
    update_metadata(0);
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    
    bytes = writeregfile(buf, size, offset, block_pointers, blocks_needed);
    fd->node->size = bytes;
    free(block_pointers);
    return bytes;
}

static int set_zeros(unsigned int *pointers, int total_blocks, int inode_num){
    int offset = inode_table[inode_num].size % SECTOR_SIZE;
    unsigned char buffer[SECTOR_SIZE];
    device_read_sector(buffer, pointers[0]);
    memset(buffer+offset, 0, SECTOR_SIZE-offset-1);
    device_write_sector(buffer, pointers[0]);
    memset(buffer, 0, SECTOR_SIZE);
    for(int i = 1; i < total_blocks; i++){
	device_write_sector(buffer, pointers[i]);
    }
    device_flush();
    return 0;
}

int p6fs_truncate(const char *path, off_t newSize)
{
    int inode_num = pathref(path, CHILD);
    int filesize = inode_table[inode_num].size;
    int blocks_needed = newSize / SECTOR_SIZE + (newSize % SECTOR_SIZE != 0);
    int blocks_ownd = filesize / SECTOR_SIZE + (filesize % SECTOR_SIZE != 0);
    if(inode_num == -ENOENT)
	return -ENOENT;
    if(newSize < 0 || (unsigned long)newSize > 3 * G)
	return -EINVAL;

    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);


    if(inode_table[inode_num].size > newSize)
	shrink(inode_num, blocks_needed);
    else if(inode_table[inode_num].size < newSize){
	extend_file(inode_num, blocks_needed);
	unsigned int *pointers = malloc(blocks_needed-blocks_ownd);
	if(pointers == NULL){
	    pthread_mutex_unlock(&bitmap_lock);
	    pthread_mutex_unlock(&inode_lock);
	    return -ENOMEM;	    
	}
	read_file_pointers(pointers, blocks_ownd-1, blocks_needed-blocks_ownd, inode_num);
	set_zeros(pointers,  blocks_needed-blocks_ownd, inode_num);
    }
    inode_table[inode_num].size = newSize;
    update_metadata(inode_num == ROOT_INODE);
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    return 0;
}

//optional
//p6fs_flush(const char *path, struct fuse_file_info *fileInfo)
//int p6fs_fsync(const char *path, int datasync, struct fuse_file_info *fi)
int p6fs_release(const char *path, struct fuse_file_info *fileInfo)
{
    /*release fd*/
    int inode_num = pathref(path, CHILD);
    if(inode_num == -ENOENT)
	return -ENOENT;

    struct inode_t inode;
    getinode(&inode, inode_num);

    int find = 0;
    for(int i = 0; i < MAX_OPEN_FILE; i++){
	if(fd_table[i].inode_num == inode_num){
	    find = 1;
	    fd_table[i].node = NULL;
	    fd_table[i].allocated = 0;
	    return 0;
	}
    }
    fileInfo->fh = 0;
    if(!find)
	return -ENOENT;
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
	    statbuf->st_mode = S_IFDIR | inode.access_mod;
	    statbuf->st_nlink = inode.nlinks;
	}
	else if(inode.mode == FILE_T){
	    statbuf->st_mode = S_IFREG | inode.access_mod;
	    statbuf->st_nlink = inode.nlinks;
	    statbuf->st_size = inode.size;
	}
	else{
	    statbuf->st_mode = S_IFLNK | inode.access_mod;
	    statbuf->st_nlink = inode.nlinks;
	    statbuf->st_size = inode.size;
	}
	statbuf->st_ctim.tv_sec = inode_table[inode_num].create_time;
	statbuf->st_atim.tv_sec = inode_table[inode_num].last_access_time;
	statbuf->st_mtim.tv_sec = inode_table[inode_num].last_modified_time;
    }
    return 0;
}

int p6fs_utime(const char *path, struct utimbuf *ubuf){
    int inode_num = pathref(path, CHILD);
    if(inode_num == -ENOENT)
	return -ENOENT;
    ubuf->actime = inode_table[inode_num].last_access_time;
    ubuf->modtime = inode_table[inode_num].last_modified_time;
    return 0;
};//optional

int p6fs_chmod(const char *path, mode_t mode){
    int inode_num = pathref(path, CHILD);
    if(inode_num == -ENOENT)
	return -ENOENT;

    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);
    
    inode_table[inode_num].access_mod = mode;
    update_metadata(inode_num == ROOT_INODE);
    device_flush();
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    return 0;
} //optional

int p6fs_chown(const char *path, uid_t uid, gid_t gid){
    return 0;
}//optional


int p6fs_rename(const char *path, const char *newpath)
{
    if(strcmp(path, newpath) == 0)
	return 0;

    int child_inode_num, old_parent_inode_num, new_parent_inode_num;
    struct inode_t child_inode, old_parent_inode, new_parent_inode;
    char child_filename[FILENAME_MAX], new_child_filename[FILENAME_MAX];

    child_inode_num = pathref(path, CHILD);
    old_parent_inode_num = pathref(path, PARENT);
    new_parent_inode_num = pathref(newpath, PARENT);

    if(child_inode_num == -ENOENT || old_parent_inode_num == -ENOENT || new_parent_inode_num == -ENOENT)
	return -ENOENT;

    getinode(&old_parent_inode, old_parent_inode_num);
    getinode(&new_parent_inode, new_parent_inode_num);
    if(new_parent_inode.size > 4 * M)
	return -ENOMEM;

    getfilename(path, child_filename);
    getfilename(newpath, new_child_filename);

    if(*new_child_filename == '\0' || *new_child_filename == '\0')
	return -ENOBUFS;
    
    pthread_mutex_lock(&bitmap_lock);
    pthread_mutex_lock(&inode_lock);    
    int status;
    status = remove_entry(&old_parent_inode, old_parent_inode_num, child_filename);
    if(status != 0){
	pthread_mutex_unlock(&bitmap_lock);
	pthread_mutex_unlock(&inode_lock);
	return status;
    }

    struct file_inode *fip = (struct file_inode*)malloc(old_parent_inode.size);
    if(fip == NULL)
	return -ENOMEM;
    memset(fip, 0, old_parent_inode.size);
    // TODO return -ENOBUF in readdir
    readdirfile(fip, old_parent_inode.size,
		old_parent_inode.direct_pointer, old_parent_inode.one_level_pointer);

    if(new_parent_inode.size % SECTOR_SIZE == 0)
	if(extend_dir(new_parent_inode_num) == -1){
	    free(fip);
	    fip = NULL;
	    pthread_mutex_unlock(&bitmap_lock);
	    pthread_mutex_unlock(&inode_lock);
	    return -ENOMEM;	    
	}

    status = create_entry(&new_parent_inode,
			  new_parent_inode_num,
			  new_child_filename,
			  child_inode_num);
    if(status != 0){
	pthread_mutex_unlock(&bitmap_lock);
	pthread_mutex_unlock(&inode_lock);
	free(fip);
	fip = NULL;
	return -EEXIST;	
    }
    update_metadata(old_parent_inode_num == ROOT_INODE || new_parent_inode_num == ROOT_INODE);
    device_flush();
    free(fip);
    fip = NULL;
    pthread_mutex_unlock(&bitmap_lock);
    pthread_mutex_unlock(&inode_lock);
    return 0;
}

int p6fs_statfs(const char *path, struct statvfs *statInfo)
{
    /*print fs status and statistics */
    statInfo->f_bavail = INODE_NUM; 
    statInfo->f_bfree  = glo_superblock.available_blocks - (INODE_BLOCKS+2);
    statInfo->f_blocks = INODE_NUM;       // total blocks
    statInfo->f_bsize  = SECTOR_SIZE;
    statInfo->f_ffree  = glo_superblock.available_inodes-1;
    statInfo->f_files  = INODE_NUM;
    return 0;
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
	fd_table[i].inode_num = -1;
    }

    rootdir.ino = 0;
    rootdir.dir_size = sb->root_dir_size;

    // read the whole directory in memory;
    // each block stores 64 entries;
    rootdir.fi = (struct file_inode*)malloc(rootdir.dir_size);  // will not be freed
    struct fuse_context *fuse_con = fuse_get_context();
    fuse_con->private_data = (void *)rootdir.fi;
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
    sb->available_blocks = INODE_NUM;                      // 1048576
    sb->available_inodes = INODE_NUM;
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
    inode_table[ROOT_INODE].create_time = time(NULL);
    inode_table[ROOT_INODE].last_access_time = inode_table[ROOT_INODE].create_time;
    inode_table[ROOT_INODE].last_modified_time = inode_table[ROOT_INODE].last_access_time;
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

    for(int i = 0; i < MAX_OPEN_FILE; i++){
	*name_hash[i].file_name = '\0';
	name_hash[i].ino = -1;
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
    device_flush();
    struct fuse_context *fuse_con = fuse_get_context();
    free(fuse_con->private_data);
    device_close();
    logging_close();
}



unsigned int path_ref(const char *path){

}
