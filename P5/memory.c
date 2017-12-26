/* Author(s): <Your name here>
 * Implementation of the memory manager for the kernel.
 */
#include "common.h"
#include "interrupt.h"
#include "kernel.h"
#include "memory.h"
#include "printf.h"
#include "scheduler.h"
#include "util.h"

#define MEM_START 0xa0908000
#define PAGE_TABLE_SEGMENT 0xa0820000
#define NEXT_PAGE_TABLE 0x400 // size of page table is at most 1KB 
/* Static global variables */
// Keep track of all pages: their vaddr, status, and other properties
static page_map_entry_t page_map[ PAGEABLE_PAGES ];

// other global variables...
// static lock_t page_fault_lock;


// Why we need following four functions
/* TODO: Returns physical address of page number i */
uint32_t page_paddr( int i ) {
    return page_map[i].paddr;
}

/* TODO: Returns virtual address (in kernel) of page number i */
uint32_t page_vaddr( int i ) {
    return page_map[i].vaddr;
}

uint32_t* page_addr(int i){
    return (uint32_t*)page_map[i].paddr;
}
/* get the physical address from virtual address (in kernel) */
uint32_t va2pa( uint32_t va ) {
    return (uint32_t) va - 0xa0000000;
}

/* get the virtual address (in kernel) from physical address */
uint32_t pa2va( uint32_t pa ) {
    return (uint32_t) pa + 0xa0000000;
}


// TODO: insert page table entry to page table
void insert_page_table_entry( uint32_t *table, uint32_t vaddr, uint32_t paddr,
                              uint32_t flag, uint32_t pid ) {

    
    // insert entry
    table[vaddr>>12] = (paddr & 0xfffff000) >> 6 | flag;
    // tlb flush
    uint32_t entry_hi = (vaddr & 0xffffe000) | (pid & 0xff);
    tlb_flush(entry_hi);
}


/* TODO: Allocate a page. Return page index in the page_map directory.
 *
 * Marks page as pinned if pinned == TRUE.
 * Swap out a page if no space is available.
 */
int page_alloc( int pinned ) {
    // code here

    int free_index;
    int find = 0;
    static int clock = 0;
    for(free_index = 0; free_index < PAGEABLE_PAGES; free_index++){
	if(page_map[free_index].unused){
	    find = 1;	    
	    break;
	}
    }

    if(!find){
	for(free_index = clock; free_index < PAGEABLE_PAGES; free_index++){
	    printf(4, 1, "searching");
	    if(!page_map[free_index].pinned){
		find = 1;
		clock = (free_index + 1) % PAGEABLE_PAGES;
		break;
	    }
	}
	int pid = page_map[free_index].pid;
	int VPN2 = page_map[free_index].vaddr & 0xffffe000;
	uint32_t* page_table = (uint32_t*)pcb[pid-1].page_table;
	page_table[page_map[free_index].vaddr>>12] = 0;
	tlb_flush(VPN2 | pid);
    }
    bzero((char*)(page_map[free_index].paddr + 0xa0000000), PAGE_SIZE);
    page_map[free_index].pid = current_running->pid;
    page_map[free_index].VPN = current_running->user_tf.cp0_badvaddr >> 12;
    page_map[free_index].vaddr = current_running->user_tf.cp0_badvaddr;
    page_map[free_index].unused = FALSE;
    page_map[free_index].pinned = pinned;
    page_map[free_index].dirty = FALSE;
    return free_index;
}

/* TODO: 
 * This method is only called once by _start() in kernel.c
 */
uint32_t init_memory( void ) {
    // initialize all pageable pages to a default state
    int i;
    printf(1, 1, "initializing %d pages", PAGEABLE_PAGES);
    for(i = 0; i < PAGEABLE_PAGES; i++){
	page_map[i].pid = 0;
	page_map[i].vaddr = 0;
	page_map[i].paddr = va2pa(MEM_START + i * 0x1000); // 4KB = 0x1000Byte
	page_map[i].VPN = 0;
	page_map[i].PFN = page_map[i].paddr >> 12;
	page_map[i].unused = 1;
	page_map[i].pinned = 0;
	page_map[i].dirty = 0;
    }
    return 0;
}

/* TODO: 
 * 
 */
uint32_t setup_page_table( int pid ) {
    uint32_t index = page_alloc(TRUE);
    
    uint32_t page_dir = pa2va(page_map[index].paddr);
    page_map[index].pid = pid;
    page_map[index].vaddr = page_dir;
    bzero((char*)page_dir, 0x1000);

    // flush entry point in TLB
    return page_dir;
}

uint32_t do_tlb_miss(uint32_t vaddr, int pid) {
    return 0;
}

void create_pte(uint32_t vaddr, int pid) {
    return;
}
