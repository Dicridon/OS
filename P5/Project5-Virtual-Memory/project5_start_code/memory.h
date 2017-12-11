/* Author(s): <Your name here>
 * Defines the memory manager for the kernel.
*/

#ifndef MEMORY_H
#define MEMORY_H

enum {
  /* physical page facts */
  PAGE_SIZE = 4096,
  PAGE_N_ENTRIES = (PAGE_SIZE / sizeof(uint32_t)),

  // Global bit
  PE_G = (0x40 >> 6),
  // Valid bit
  PE_V = (0x80 >> 6),
  // Writable bit
  PE_D = (0x100 >> 6),
  // Uncache bit
  PE_UC = (0x400 >> 6),

  /* Constants to simulate a very small physical memory. */
  PAGEABLE_PAGES = 256,

  CLEAR_PAGE_OFFSET = (0xfffff000),
};

/* TODO: Structure of an entry in the page map */
typedef struct {
    // design here
    uint32_t VPN;
    uint32_t PFN;    // immutable
    uint32_t paddr;  // immutable
    uint32_t vaddr;
    pid_t pid;
    bool_t unused;
    bool_t dirty;
    bool_t valid;
    bool_t pinned;
    bool_t global;
    bool_t UC;
} page_map_entry_t;


/* Prototypes */

/* Return the physical address of the i-th page */
uint32_t* page_addr(int i);

/* Allocate a page.  If necessary, swap a page out.
 * On success, return the index of the page in the page map.  On
 * failure, abort.  BUG: pages are not made free when a process
 * exits.
 */
int page_alloc(int pinned);

/* init page_map */
uint32_t init_memory(void);

/* Set up a page directory and page table for the given process. Fill in
 * any necessary information in the pcb. 
 */
uint32_t setup_page_table(int pid);

// other functions defined here
//

#endif /* !MEMORY_H */
