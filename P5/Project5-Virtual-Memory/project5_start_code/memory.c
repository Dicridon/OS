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
    uint32_t virtual_page_number = (vaddr) >> 13;
    uint32_t page_tabel_index = virtual_page_number;
    uint32_t physical_page_number = (paddr) >> 12;
    uint32_t PTE = 0;
    uint32_t entry_hi = 0;
    entry_hi |= ((virtual_page_number << 13) | pid);
    PTE |=  ((physical_page_number << 12) | (flag));
    table[page_tabel_index] = PTE;
    // tlb flush
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
    for(free_index = 0; free_index < PAGEABLE_PAGES; free_index++){
	if(page_map[free_index].unused == TRUE && page_map[free_index].pinned == FALSE)
	    break;
    }

    if(free_index >= PAGEABLE_PAGES)
	ASSERT2(-1, "No space available");
    page_map[free_index].pid = current_running->pid;
    page_map[free_index].VPN = current_running->user_tf.cp0_badvaddr >> 12;
    page_map[free_index].vaddr = current_running->user_tf.cp0_badvaddr;
    page_map[free_index].unused = FALSE;
    page_map[free_index].pinned = pinned;
    page_map[free_index].valid = TRUE;
    page_map[free_index].dirty = FALSE;
    page_map[free_index].global = FALSE;
    page_map[free_index].UC = FALSE;
    return free_index;
}

/* TODO: 
 * This method is only called once by _start() in kernel.c
 */
uint32_t init_memory( void ) {
    
    // initialize all pageable pages to a default state
    int i;
    for(i = 0; i < PAGEABLE_PAGES; i++){
	page_map[i].pid = 0;
	page_map[i].vaddr = 0;
	page_map[i].paddr = va2pa(MEM_START + i * 0x1000); // 4KB = 0x1000Byte
	page_map[i].VPN = 0;
	page_map[i].PFN = page_map[i].paddr >> 12;
	page_map[i].unused = 1;
	page_map[i].pinned = 0;
	page_map[i].valid = 0;
	page_map[i].dirty = 0;
	page_map[i].global = 0;
	page_map[i].UC = 0;
    }
}

/* TODO: 
 * 
 */
uint32_t setup_page_table( int pid ) {
    static uint32_t page_table_start = PAGE_TABLE_SEGMENT;
    uint32_t page_table;
    // alloc page for page table 
    // Since I store all page tables in kernel space
    // I don't need to alloc space for page table
    // space needed is mapped to physical space by
    // substructing 0xa00000000 from vaddr
    page_table = page_table_start;
    page_table_start += NEXT_PAGE_TABLE;
    ASSERT(page_table_start < MEM_START);
   
    // initialize PTE and insert several entries into page tables using insert_page_table_entry
    insert_page_table_entry((uint32_t*)page_table,
			    pcb[pid-1].entry_point,
			    0,
			    0,
			    pid);

    return page_table;
}

uint32_t do_tlb_miss(uint32_t vaddr, int pid) {
    return 0;
}

void create_pte(uint32_t vaddr, int pid) {
    return;
}
