#include "common.h"
#include "interrupt.h"
#include "queue.h"
#include "scheduler.h"
#include "sync.h"
#include "util.h"
#include "printf.h"

lock_t kernel_locks[32];

/*TODO operate */
static bool_t unblock_one(node_t *wait_enqueue)
 {
	ASSERT(current_running->nested_count);

	pcb_t *pcb = NULL; 
	pcb = (pcb_t *)dequeue(wait_enqueue);
	if(pcb != NULL)
	{
	    unblock(pcb);
	    return TRUE;
	}
	return FALSE;
}

static bool_t unblock_all(node_t *wait_enqueue)
{
    while(!is_empty(wait_enqueue))
	unblock_one(wait_enqueue);
}

/* Initialize a lock */
void lock_init(lock_t * l){
    /* No critical section; it is the caller's responsibility to make sure
       that locks are initialized only once */
    l->node.next = &l->node;
    l->node.prev = &l->node;
    l->status = UNLOCKED;
    queue_init(&l->wait_queue);
    l->held_task = NULL;
}

void do_lock_init(int l){
    if(l >= 0 && l <= 31)
	lock_init(&kernel_locks[l]);
}

static int lock_acquire_helper(lock_t * l){
    ASSERT(disable_count);
    if (LOCKED == l->status) {
	current_running->blocking_lock = (void*)l;
	pcb_t* cur_task = current_running;
	lock_t* cur_lock;
	while (cur_task){
	    pcb_t* tmp_task;
	    cur_lock = (lock_t*)cur_task->blocking_lock;
	    if (cur_lock){
		tmp_task = cur_lock->held_task;
		if (tmp_task==current_running){
		    current_running->blocking_lock = NULL;
		    return 1;
		}
	    }
	    else break;
	    cur_task = tmp_task;
	}
	block(&l->wait_queue);
	current_running->blocking_lock = NULL;
    } else {
	l->status = LOCKED;
    }

//    // customization
//    enqueue(&current_running->lq, (node_t*)l);
    return 0;
}

// Return 0 on succes
// Return 1 on failure due to deadlock (extra credit)
int lock_acquire(lock_t * l){
    enter_critical();
    int result = lock_acquire_helper(l);
    leave_critical();

    return result;
}

int do_lock_acquire(int l){
    if(l >= 0 && l <= 31)
	return lock_acquire(&kernel_locks[l]);
    else
	return -1;
}

static void lock_release_helper(lock_t * l){
    ASSERT(disable_count);
    if (!unblock_one(&l->wait_queue)) {
	l->status = UNLOCKED;
    }
    l->held_task = NULL;

    // customization
    node_t* lk;
    node_t* lq = &current_running->lq;
    int i = 3;
    printf((i++)+5, 1, "lq %x ", lq);
    for(lk = peek(lq); lk != NULL && lk != lq; lk = lk->next){
	printf(i+5, 1, "lk %x ", lk);
	printf(i+6, 1, "lk %x ", lk->prev);
	printf(i+7, 1, "lk %x ", lk->next);
	i++;
	if((lock_t*)lk == l){
	    lk->prev->next = lk->next;
	    lk->next->prev = lk->prev;
	    break;
	}
    }
}

void lock_release(lock_t * l){
    enter_critical();
    lock_release_helper(l);
    leave_critical();
}

void do_lock_release(int l){
    if(l >= 0 && l <= 31)
	lock_release(&kernel_locks[l]);
}

/* TODO: Initialize a condition variable */
void condition_init(condition_t * c){
    queue_init(&c->wait_queue);
}

/* TODO: Release lock m and block the thread (enqueued on c).  When unblocked,
   re-acquire m */
/* error */
void condition_wait(lock_t * m, condition_t * c){
    enter_critical();
    lock_release_helper(m);
    current_running->status = BLOCKED;
    block(&c->wait_queue);

//    scheduler_entry();

    lock_acquire_helper(m);
    leave_critical();
}

/* TODO: Unblock the first thread waiting on c, if it exists */
void condition_signal(condition_t * c){
    enter_critical();
    if(!is_empty(&c->wait_queue))
	unblock_one(&c->wait_queue);
    leave_critical();
}

/* TODO: Unblock all threads waiting on c */
void condition_broadcast(condition_t * c){
    enter_critical();
    if(!is_empty(&c->wait_queue))
	unblock_all(&c->wait_queue);
    leave_critical();
}

/* TODO: Initialize a semaphore with the specified value. value must be >= 0 */
void semaphore_init(semaphore_t * s, int value){
    ASSERT(value);
    s->semaphore_value = value;
    queue_init(&s->wait_queue);
}

/* TODO: Increment the semaphore value atomically */
void semaphore_up(semaphore_t * s){
    enter_critical();

//    if(s->semaphore_value++ <= 0 && !is_empty(&s->wait_queue))
    s->semaphore_value++;
    if(!is_empty(&s->wait_queue))
	unblock_one(&s->wait_queue);
    
    leave_critical();
}

/* TODO: Block until the semaphore value is greater than zero and decrement it */
void semaphore_down(semaphore_t * s){
    enter_critical();
    // if intteruptable, decrement and comparison may be separated
    // so a process may not be blocked even though semaphore value
    // is already negative
    if(s->semaphore_value == 0)
	block(&s->wait_queue);
    if(s->semaphore_value > 0)
	s->semaphore_value--;

    leave_critical();
}

/* TODO: Initialize a barrier.  n is number of threads that rendezvous at the
   barrier */
void barrier_init(barrier_t * b, int n){
    ASSERT(n);
    b->target = n;
    b->blocked = 0;
    queue_init(&b->wait_queue);
}

/* TODO: Block until all n threads have called barrier_wait */
void barrier_wait(barrier_t * b){
    enter_critical();
    if(++b->blocked == b->target){
	unblock_all(&b->wait_queue);
	b->blocked = 0;
    }
    else
	block(&b->wait_queue);
    leave_critical();
}
