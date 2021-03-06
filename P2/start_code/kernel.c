/*
  kernel.c
  the start of kernel
*/

#include "common.h"
#include "kernel.h"
#include "scheduler.h"
#include "th.h"
#include "util.h"
#include "queue.h"

#include "tasks.c"

volatile pcb_t *current_running;

queue_t ready_queue, blocked_queue;
struct queue r_q, b_q;
pcb_t *ready_arr[NUM_TASKS];
pcb_t *blocked_arr[NUM_TASKS];
pcb_t pcb_table[NUM_TASKS];
struct context context_arr[NUM_TASKS];

/*
  this function is the entry point for the kernel
  It must be the first function in the file
*/

#define PORT3f8 0xbfe48000

void printnum(unsigned long long n)
{
    int i,j;
    unsigned char a[40];
    unsigned long port = PORT3f8;
    i=10000;
    while(i--);

    i = 0;
    do {
	a[i] = n % 16;
	n = n / 16;
	i++;
    }while(n);

    for (j=i-1;j>=0;j--) {
	if (a[j]>=10) {
	    *(unsigned char*)port = 'a' + a[j] - 10;
	}else{
	    *(unsigned char*)port = '0' + a[j];
	}
    }
    printstr("\r\n");
}

void _stat(void){

    /* some scheduler queue initialize */
    /* TODO */
    r_q.head = 0;
    r_q.tail = 0;
    r_q.isEmpty = TRUE;
    r_q.capacity = NUM_TASKS;
    r_q.pcbs = ready_arr;
    ready_queue = &r_q;

    b_q.head = 0;
    b_q.tail = 0;
    b_q.isEmpty = TRUE;
    b_q.capacity = NUM_TASKS;
    b_q.pcbs = blocked_arr;
    blocked_queue = &b_q;


    /* Initialize the PCBs and the ready queue */
    /* TODO */
    /* need student add */
    
    // PCB is maintained by kernel;
    int i;
    for(i = 0; i < NUM_TASKS; i++){
	pcb_table[i].pid = i+1;
	pcb_table[i].pstate = PROCESS_READY;
	pcb_table[i].context = &context_arr[i];
	pcb_table[i].context->sp = STACK_MAX-(STACK_SIZE*i);
	pcb_table[i].context->ra = task[i]->entry_point;
	pcb_table[i].context->s0 = 0;
	pcb_table[i].context->s1 = 0;
	pcb_table[i].context->s2 = 0;
	pcb_table[i].context->s3 = 0;
	pcb_table[i].context->s4 = 0;
	pcb_table[i].context->s5 = 0;
	pcb_table[i].context->s6 = 0;
	pcb_table[i].context->s7 = 0;
	pcb_table[i].context->fp = pcb_table[i].context->sp;
	printnum(pcb_table[i].context->ra);
	queue_push(ready_queue, &pcb_table[i]);
    }
    printstr("14:20");
    clear_screen(0, 0, 30, 24);
    
    /*Schedule the first task */
    /*
      the procedure is simple
      _stat -> schedule_entry -> scheduler -> save_pcb
      use current_process to set new process
    */

    scheduler_count = 0;
    scheduler_entry();
    
    /*We shouldn't ever get here */
    ASSERT(0);
}
