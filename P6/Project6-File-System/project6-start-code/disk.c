#include <stdlib.h>
#include <stdio.h>

#include "disk.h"

static const char *device_path;
static FILE *f;

int device_open(const char *path)
{
    int ret;
    device_path = path;
    f = fopen(path, "r+");
    ret = f != NULL ? 0 : -1;
    
    return ret;
}

void device_close()
{
    fflush(f);
    fclose(f);
}

int device_read_sector(unsigned char buffer[], int sector)
{
    fseek(f, sector*SECTOR_SIZE, SEEK_SET);
    
    return (fread(buffer, 1, SECTOR_SIZE, f) == SECTOR_SIZE);
}

int device_write_sector(unsigned char buffer[], int sector)
{
    fseek(f, sector*SECTOR_SIZE, SEEK_SET);
    
    return (fwrite(buffer, 1, SECTOR_SIZE, f) == SECTOR_SIZE);
}

void device_flush()
{
    fflush(f);
}

