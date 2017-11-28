#include "common.h"
#include "scheduler.h"
#include "sync.h"
#include "util.h"
#include "printf.h"

/* Threads to test barrier */

lock_t lock1;
lock_t lock2;
bool_t locks_init = FALSE;
pid_t pid1;
pid_t pid2;
// hold a lock and yield and try
// to obtain another lock
void barrier1(void)
{
    for(;;){
	pid1 = current_running->pid;
	
	if(!locks_init){
	    lock_init(&lock1);
	    lock_init(&lock2);
	    locks_init = TRUE;
	}
	printf(1, 1, "(%d) I will try to hold lock1      ", current_running->pid);
	do_sleep(3000);
	lock_acquire(&lock1);
	printf(1, 1, "(%d) lcok1 is mine!!!!!!!!!!!      ", current_running->pid);
	do_yield();
	printf(2, 1, "(%d) Now I will try to hold lock2  ", current_running->pid);
	lock_acquire(&lock2);
	printf(2, 1, "(%d) lock2 is mine!!!!!!!!!!!      ", current_running->pid);
	lock_release(&lock1);
	lock_release(&lock2);
    }
}

void barrier2(void)
{
    for(;;){
	pid2 = current_running->pid;
	
	printf(3, 1, "(%d) I will try to hold lock2      ", current_running->pid);
	do_sleep(3000);
	lock_acquire(&lock2);
	printf(3, 1, "(%d) lock2 is mine!!!!!!!!!!!      ", current_running->pid);
	do_yield();
	printf(4, 1, "(%d) Now I will try to hold lock1  ", current_running->pid);
	lock_acquire(&lock1);
	printf(4, 1, "(%d) lock1 is mine!!!!!!!!!!!      ", current_running->pid);
	lock_release(&lock2);
	lock_release(&lock1);
    }
}

// this thread kill one of the two threads to
// solve deadlock
void barrier3(void)
{
//    int deadlock1 = 0;
//    int deadlock2 = 0;
    printf(7, 1, "I am the deadlock killer     ");
    for(;;){
//	deadlock1 = (lock1.held_task->pid == pid1 &&
//		     lock2.held_task->pid == pid2 &&
//		     ((pcb_t*)lock1.wait_queue.next)->pid == pid2 &&
//		     ((pcb_t*)lock2.wait_queue.next)->pid == pid1);
//
//	deadlock2 = (lock1.held_task->pid == pid2 &&
//		     lock2.held_task->pid == pid1 &&
//		     ((pcb_t*)lock1.wait_queue.next)->pid == pid1 &&
//		     ((pcb_t*)lock2.wait_queue.next)->pid == pid2);

	printf(7, 1, "waiting for deadlock    ");
	do_sleep(1000);
	printf(7, 1, "waiting for deadlock.   ");
	do_sleep(1000);
	printf(7, 1, "waiting for deadlock..  ");
	do_sleep(1000);
	printf(7, 1, "waiting for deadlock...  ");
	do_sleep(1000);
	printf(7, 1, "deadlock deteced        ");
	printf(7, 1, "kill %d                 ", pid1);
	do_kill(pid1);
	printf(7, 1, "no deadlock now         ");
	do_spawn("barrier1");
	do_yield();
    }
}
