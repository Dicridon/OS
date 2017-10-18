/* kernel.h: definitions used by kernel code */

#ifndef KERNEL_H
#define KERNEL_H

#define NUM_REGISTERS 8

#include "common.h"

/* ENTRY_POINT points to a location that holds a pointer to kernel_entry */
#define ENTRY_POINT ((void (**)(int)) 0xa080fff8)

/* System call numbers */
enum {
    SYSCALL_YIELD,
    SYSCALL_EXIT,
};

/* All stacks should be STACK_SIZE bytes large
 * The first stack should be placed at location STACK_MIN
 * Only memory below STACK_MAX should be used for stacks
 */
enum {
    STACK_MIN = 0xa0840000,
    STACK_SIZE = 0x2000,
    STACK_MAX = 0xa0890000,
};

typedef enum {
    PROCESS_BLOCKED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_EXITED,
} process_state;


typedef struct context{
    uint32_t sp;
    uint32_t ra;
    uint32_t s0;
    uint32_t s1;
    uint32_t s2;
    uint32_t s3;
    uint32_t s4;
    uint32_t s5;
    uint32_t s6;
    uint32_t s7;
    uint32_t fp;
}* context_t;


typedef struct pcb {
    /* need student add */
    /* TODO */
    uint32_t pid;
    process_state pstate;
    context_t context;
} pcb_t;

/* The task currently running.  Accessed by scheduler.c and by entry_mips.s assembly methods */
extern volatile pcb_t *current_running;

void kernel_entry(int fn);

#endif                          /* KERNEL_H */
