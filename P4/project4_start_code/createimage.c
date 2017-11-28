#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define SECTOR 512
#define PROCESS_BLOCK 128

#define PROCESS1_OFF 0x20000
#define PROCESS2_OFF 0x22000
#define PROCESS3_OFF 0x24000
#define PROCESS4_OFF 0x26000


// Open a file, return its program header and pass out a Elf header
Elf32_Phdr * read_exec_file(FILE **execfile, char *filename, Elf32_Ehdr **ehdr)
{
    
    Elf32_Phdr* phdr = malloc(sizeof(Elf32_Phdr));
    if(phdr == NULL){
	puts("read_exec_file, Line15: Mallocation failed");
	return 0;
    }
    
    *execfile = fopen(filename, "rb");
    if(*execfile == NULL){
	printf("read_exec_file, Line21: Openning %s failed\n", filename);
	return 0;	
    }
    *ehdr = malloc(sizeof(Elf32_Ehdr));
    if(*ehdr == NULL){
	puts("read_exec_file, Line26: Mallocation failed");
	return 0;
    }
    fread(*ehdr, sizeof(Elf32_Ehdr), 1, *execfile);

    
    fseek(*execfile, (*ehdr)->e_phoff, SEEK_SET);
    fread(phdr, sizeof(Elf32_Phdr), 1, *execfile);

    fclose(*execfile);
    return phdr;
}

// read a bootblock and write it into an image
void write_bootblock(FILE *image_file, char* bootname, Elf32_Phdr *boot_phdr)
{
    if(bootname == NULL)
	bootname = "bootblock";
    
    FILE* boot_file = fopen(bootname, "rb");
    if(boot_file == NULL){
	printf("write_bootblock, Line47: Error openging file %s in\n", bootname);
	return;
    }
	 
	 
    if(image_file == NULL)
	return ;
    char filling[SECTOR] = "";  
    char buffer[SECTOR] = "";
    uint16_t fill = 0x55aa;  
	 
    // find the program segment
    fseek(boot_file, boot_phdr->p_offset, SEEK_SET); 
    fread(buffer, sizeof(char), boot_phdr->p_filesz, boot_file);
    fwrite(buffer, boot_phdr->p_filesz, 1, image_file);
	 
    if(boot_phdr->p_filesz % SECTOR != 0){
	fwrite(filling, sizeof(char),  SECTOR-2-boot_phdr->p_filesz%SECTOR, image_file);
	fwrite(&fill, 1, 2, image_file);
    }
	 
    fclose(boot_file);
}


// Open a kernel file and wirte into an image.
void write_kernel(FILE *image_file, char *kernelname, Elf32_Ehdr *kernel_ehdr)
{
    FILE* kernel_file = fopen(kernelname, "rb");
    if(kernel_file == NULL){
	printf("write_kernel, Line74: Error openning file %s\n", kernelname);
	return;
    }
    

    if(image_file == NULL)
	return ;
    char* buffer;
    char filling[SECTOR] = "";

    int i;
    int total_size = 0;
    Elf32_Phdr* phdr_table = (Elf32_Phdr*)malloc(sizeof(Elf32_Phdr) * kernel_ehdr->e_phnum);

    fseek(kernel_file, kernel_ehdr->e_phoff, SEEK_SET);
    fread(phdr_table, sizeof(Elf32_Phdr), kernel_ehdr->e_phnum, kernel_file);
    
    for(i = 0; i < kernel_ehdr->e_phnum; i++){
	printf("write_kernel, Line92: %dth phdr, offset: 0x%x, size: 0x%x\n",
	       i, phdr_table[i].p_offset, phdr_table[i].p_filesz);
	
	fseek(kernel_file, phdr_table[i].p_offset, SEEK_SET);
	total_size += phdr_table[i].p_filesz;
    }
    buffer = (char*)malloc(sizeof(char) * total_size);

    for(i = 0; i < kernel_ehdr->e_phnum; i++){
	fseek(kernel_file, phdr_table[i].p_offset, SEEK_SET);
	fread(buffer, phdr_table[i].p_filesz, 1, kernel_file);
    }
    
    fwrite(buffer, total_size, 1, image_file);

    if(total_size % SECTOR != 0){
	fwrite(filling, sizeof(char), SECTOR-total_size%SECTOR, image_file);
    }


    fclose(kernel_file);
    free(buffer);
    free(phdr_table);
}

int count_kernel_sectors(char *kernel, Elf32_Ehdr *kernel_ehdr){
    int i;
    int total_size = 0;

    FILE* kernel_file = fopen(kernel, "rb");
    if(kernel_file == NULL){
	puts("count_kernel_sectors, Line122: kernel file is NULL");
	return -1;
    }

    Elf32_Phdr* kernel_phdr = malloc(sizeof(Elf32_Phdr));
    if(kernel_phdr == NULL){
	puts("count_kernel_sectors, Line128: mallocation failed");
	return -1;
    }
    for(i =0; i < kernel_ehdr->e_phnum; i++){
	fseek(kernel_file, kernel_ehdr->e_phoff+i, SEEK_SET );
	fread(kernel_phdr, sizeof(Elf32_Phdr), 1, kernel_file);
	total_size += kernel_phdr->p_filesz;
    }
    free(kernel_phdr);
    fclose(kernel_file);
    return total_size/SECTOR + (total_size % SECTOR != 0);
}


void record_kernel_sectors(FILE *image_file, uint32_t sector_size , uint32_t total_size){
    if(image_file == NULL){
	printf("image file can not open\n");
	return;
    }

    fseek(image_file, 0x9c, SEEK_SET);
    fwrite(&total_size, 1, 4, image_file);
}


uint32_t write_process(FILE* image_file, char* process_name, int process_off){
    char buffer[SECTOR];
//    char filling[SECTOR] = "";
    int file_size;
    int chunk = SECTOR;
    int read_size;
    int sectors_of_file;

    FILE* process = fopen(process_name, "rb");
    if(process == NULL){
	printf("Open process %s failed\n", process_name);
	return -1;
    }

    Elf32_Ehdr p_ehdr;
    Elf32_Phdr p_phdr;
    fread(&p_ehdr, sizeof(Elf32_Ehdr), 1, process);
    fseek(process, p_ehdr.e_phoff, SEEK_SET);
    fread(&p_phdr, sizeof(Elf32_Phdr), 1, process);
    fseek(process, p_phdr.p_offset, SEEK_SET);

    printf("write_process, Line208, ready to write **** %s ****\n", process_name);
    file_size = p_phdr.p_filesz;
    sectors_of_file = file_size/SECTOR + !(file_size % SECTOR == 0);
    printf("file size: %u\n", p_phdr.p_filesz);
    printf("file vddr: 0x%x\n", p_phdr.p_vaddr);
    printf("file sectors %u\n", sectors_of_file);


    // find desired offset and start writing
    fseek(image_file, process_off, SEEK_SET);
    while(file_size > 0){
	read_size = (chunk <= file_size) ? chunk : file_size;
	fread(buffer, 1, read_size, process);
	fwrite(buffer, 1, read_size, image_file);
	file_size -= chunk;
    }

    if(fclose(process) != 0)
	printf("close failed\n");
    else
	printf("file %s closed\n", process_name);

    return p_phdr.p_filesz;
}


void extended_opt(char* filename, int sector_num, Elf32_Ehdr* ehdr, Elf32_Phdr* phdr){
    if(filename == NULL || ehdr == NULL || phdr == NULL)
	return;
    
    printf("Imformation of %s:\n", filename);
    printf("sectors: %d\n", sector_num);
    printf("segment offset in file: 0x%x\n", phdr->p_offset);
    printf("virtual addredd: 0x%x\n", phdr->p_vaddr);
    printf("segment size in file: 0x%x\n", phdr->p_filesz);
    printf("segment size in memory: 0x%x\n", phdr->p_memsz);
    printf("size of the writting to OS image: 0x%x\n", phdr->p_filesz);
    printf("padding: 0x%x\n", SECTOR);
}


int main(int argc, char *argv[]){
    FILE* bootblockfile;
    FILE* kernelfile;
    FILE* image_file = fopen("image", "wb");
    char* bootblock = NULL;
    char* kernel = NULL;
    char* process1;
    char* process2;
    char* process3;
    char* processes;
    uint16_t sector_num;
    uint32_t process_size;
    Elf32_Ehdr* b_ehdr;
    Elf32_Phdr* b_phdr;
    Elf32_Ehdr* k_ehdr;
    Elf32_Phdr* k_phdr;


    if(argc <= 2)
	return 0;
    else if(argc == 3 && *argv[1] == '-')
	return 0;
    else if(argc == 3 && *argv[1] != '-'){
	bootblock = argv[1];
	kernel = argv[2];
	// write bootblock
	puts("generating image");
	printf("reading %s\n", bootblock);
	b_phdr = read_exec_file(&bootblockfile, bootblock, &b_ehdr);
	write_bootblock(image_file, bootblock, b_phdr);

	// write kernel
	printf("reading %s\n", kernel);
	k_phdr = read_exec_file(&kernelfile, kernel, &k_ehdr);
	write_kernel(image_file, kernel, k_ehdr);
	sector_num = count_kernel_sectors(kernel, k_ehdr);
	record_kernel_sectors(image_file, SECTOR, sector_num);

	fclose(image_file);
    }
    else if(argc == 8 && !strcmp(argv[1], "--extended")){
	bootblock = argv[2];
	kernel = argv[3];
	process1 = argv[4];
	process2 = argv[5];
	process3 = argv[6];
	processes = argv[7];
	// write bootblock
	printf("reading %s\n", bootblock);
	b_phdr = read_exec_file(&bootblockfile, bootblock, &b_ehdr);
	write_bootblock(image_file, bootblock, b_phdr);

	// write kernel
	printf("reading %s\n", kernel);
	k_phdr = read_exec_file(&kernelfile, kernel, &k_ehdr);
	sector_num = count_kernel_sectors(kernel, k_ehdr);
	write_kernel(image_file, kernel, k_ehdr);	


	printf("writing %s\n", process1);
	write_process(image_file, process1, PROCESS1_OFF);

	printf("writing %s\n", process2);
	process_size = write_process(image_file, process2, PROCESS2_OFF);

	printf("writing %s\n", process3);
	process_size = write_process(image_file, process3, PROCESS3_OFF);

	printf("writing %s\n", processes);
	process_size = write_process(image_file, processes, PROCESS4_OFF);

	
	record_kernel_sectors(image_file, SECTOR, process_size+0x26000);	
	extended_opt(bootblock, 1, b_ehdr, b_phdr);
	extended_opt(kernel, sector_num, k_ehdr, k_phdr);

	fclose(image_file);
    }
    else
	return 0;
}

