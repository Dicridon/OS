#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*********************************************************************

#define EI_NIDENT 16

Elf32_Addr 4
Elf32_Half 2
Elf32_Off 4
Elf32_Sword 4
Elf32_Word 4
unsigned char


typedef struct{
    unsigned char e_ident[EI_NIDENT];
    // ELF Identifiatoin
    // e_ident[EI_MAG0] = 0x7f
    // e_ident[EI_MAG1] = 'E'
    // e_ident[EI_MAG2] = 'L'
    // e_ident[EI_MAG3] = 'F'

    // e_ident[EI_CLASS]: 0, Invalid class
    //                    1, 32-bit objects
    //                    2, 64-bit objects
    // e_ident[EI_PAD], unused butes, reading
    // programs should ignore them

    
    Elf32_Half e_type;
    // ET_NONE 0 no file type
    // ET_REL  1 Relocatable file
    // ET_EXEC 2 Executable file
    // ET_DYN  3 Shared object file
    // ET_CORE 4 Core file
    // ET_LOPROC 0xff00 Processor-specific
    // ET_HIPROC 0xffff Processor-specific
    
    Elf32_Half e_machine;
    Elf32_Word e_version;
    
    Elf32_Addr e_entry;
    // gives the virtual address to which the system first transfers control
    // thus starting the processor.

    Elf32_Off e_phoff; *** IMPORTANT ***
    // program header table's file to offset in bytes
    
    Elf32_Off e_shoff;
    // section header table's file to offset in bytes
    
    Elf32_Word e_flags;
    // processor-specific flags associated with the file
    
    Elf32_Half e_ehsize;
    // ELF header's size in bytes
    
    Elf32_Half e_phentsize;
    // size in bytes of one entry in the file's program header table;
    // all entries are the same size

    Elf32_Half e_phnum;  *** IMPORTANT ***
    // number of entries in the program header table
    // if no program header table, e_phnum = 0

    Elf32_Half e_shentsize;
    // size in bytes of onoe entry in the file's section header table;
    // all entries are the same size
    
    Elf32_Half e_shnum;
    // number of entries in the section header table
    // if no section header, e_shnum = 0
    
    Elf32_Half e_shstrndx;
    // section header table index of the entry associated with the sectin name
    // string table
    
} Elf32_Ehdr; // ELF32 Header


typedef struct{
    Elf32_Word p_type;
    // kind of segment
    // PT_NULL,  unused, other members aare undefined
    // PT_LOAD, loadable segment
    // PT_DYNAMIC, dynamic linking information
    // PT_PHDR, location and size of the program header table itself

    
    Elf32_Off p_offset; *** IMPORTANT *** 
    // offset from hte beginning of them fileat which the first byte
    // of the segment resides
    
    Elf32_Addr p_vaddr;
    // virtual address at which the first butes of the segment
    // resides in memory
    
    Elf32_Addr p_paddr;
    // System V ignore physical adressing
    
    Elf32_Word p_filesz; *** IMPORTANT ***
    // number of bytes in the file image of the segment
    
    Elf32_Word p_memsz; 
    // number of bytes in the memory image of the segment
    
    Elf32_Word p_flags;
    // 
    Elf32_Word p_align;
    
} Elf32_Phdr;


FILE* fopen(const char* path, const char* mode);

int fclose(FILE* stream)

size_t fwrite(const void* ptr, size_t size, size_t nmemb, FILE *stream);

int fseek(FILE* stream, long offset, int whence);
// stream, input file
// offset, offset of start point of reading
// whence, SEEK_SET, from the start
           SEEK_CUR, from current point
           SEEK_END, to the end of file and add offest

size_t fread(void* buff, sizt_t size, size_t, count, FILE* stream);

*********************************************************************/

Elf32_Phdr * read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr)
{
    Elf32_Phdr* phdr = malloc(sizeof(Elf32_Phdr));
    if(phdr == NULL){
	puts("Mallocation failed");
	return 0;
    }
    
    *execfile = fopen(filename, "r");
    if(*execfile == NULL){
	printf("Openning %s failed\n", filename);
	return 0;	
    }
    *ehdr = malloc(sizeof(Elf32_Ehdr));
    if(*ehdr == NULL){
	puts("Mallocation failed");
	return 0;
    }
    fread(*ehdr, sizeof(Elf32_Ehdr), 1, *execfile);
    printf("e_phnum: %d, p_phoff: %u\n", (*ehdr)->e_phnum, (*ehdr)->e_phoff);
    
    fseek(*execfile, (*ehdr)->e_phoff, SEEK_SET);
    fread(phdr, sizeof(Elf32_Phdr), 1, *execfile);
    printf("p_offset: %u, p_filesz: %u\n", phdr->p_offset, phdr->p_filesz);
	printf("p_vaddr: %x\n", phdr->p_vaddr);
    return phdr;
}

void write_bootblock(FILE **image_file, FILE *boot_file, Elf32_Ehdr *boot_header,
		Elf32_Phdr *boot_phdr)
{
    
}

void write_kernel(FILE **image_file, FILE *kernel_file, Elf32_Ehdr *kernel_ehdr, Elf32_Phdr *kernel_phdr)
{
}

int count_kernel_sectors(Elf32_Ehdr *kernel_header, Elf32_Phdr *kernel_phdr){
}

void record_kernel_sectors(FILE **image_file, Elf32_Ehdr *kernel_ehdr, Elf32_Phdr *kernel_phdr, int num_sec){

}

void extended_opt(Elf32_Phdr *boot_phdr, int k_phnum, Elf32_Phdr *kernel_phdr, int num_sec){
}


int main(int argc, char *argv[]){

    if(argc <= 1)
	return 0;
    
    FILE* execfile;
    char* filename = argv[1];
    Elf32_Ehdr* ehdr;
    
    read_exec_file(&execfile, filename, &ehdr);
    
}
