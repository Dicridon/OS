#ifndef DISK_H
#define DISK_H

#include <sys/types.h>

#define SECTOR_SIZE 4096   // use 4k block size

// NOT define a MAX value, keep carefully for your disk R/W

int device_open(const char *path);
void device_close();
int device_read_sector(unsigned char buffer[], int sector);
int device_write_sector(unsigned char buffer[], int sector);
void device_flush();


#endif
