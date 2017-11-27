#include "common.h"
#include "mbox.h"
#include "sync.h"
#include "scheduler.h"
#include "util.h"


// I don't know why I need this void function
// if you delete it, error occurs.
void garbage();

typedef struct
{
    /* TODO */
    char message[MAX_MESSAGE_LENGTH];
} Message;

typedef struct
{
    /* TODO */
    node_t node;
    Message letters[MAX_MBOX_LENGTH];
    char name[MBOX_NAME_LENGTH + 1]; 
    int number_of_users;      // if zero, restore this mailbox
    int unused;
    uint32_t write_letter;
    uint32_t read_letter;
    node_t read_wait_queue;   // process reading an empty mailbox should be blocked
    node_t write_wait_queue;  // process writing a full mailbox should be blocked
    int empty;
    int full;
    lock_t access_lock;
} MessageBox;


static MessageBox MessageBoxen[MAX_MBOXEN];
lock_t BoxLock;

/* Perform any system-startup
 * initialization for the message
 * boxes.
 */
void init_mbox(void)
{
    /* DONE */
    int i;
    for(i = 0; i < MAX_MBOXEN; i++){
	lock_init(&MessageBoxen[i].access_lock);
	MessageBoxen[i].name[0] = '\0';
	MessageBoxen[i].number_of_users = 0;
	MessageBoxen[i].unused = 1;
	MessageBoxen[i].write_letter = 0;
	MessageBoxen[i].read_letter = 0;
	MessageBoxen[i].empty = 1;
	MessageBoxen[i].full = 0;
	queue_init(&MessageBoxen[i].read_wait_queue);
	queue_init(&MessageBoxen[i].write_wait_queue);
    }
}

/* Opens the mailbox named 'name', or
 * creates a new message box if it
 * doesn't already exist.
 * A message box is a bounded buffer
 * which holds up to MAX_MBOX_LENGTH items.
 * If it fails because the message
 * box table is full, it will return -1.
 * Otherwise, it returns a message box
 * id.
 */
// mbox_t is int
mbox_t do_mbox_open(const char *name)
{
    (void)name;
    // debug, delete later
    static int boxxxxx = 0;


    // real code
    /* TODO */
    mbox_t i;
    char* boxname = name;
    mbox_t empty_box = boxxxxx++;

    if(name == '\0')
	return -1;

//    print_str(7, 1, "openning mailbox");
//    print_hex(8, 1, (int)current_running->entry_point);
//    print_int(9, 1, (int)current_running->nested_count);
//    print_str(10, 1, "looking for a mailbox");

    for(i = 0; i < MAX_MBOXEN; i++){
	if(MessageBoxen[i].unused)
	    empty_box = i;


	// I thought this block is wrong
	// but the problem should be in the block below line 109
	if(same_string(MessageBoxen[i].name, boxname)){   
	    MessageBoxen[i].number_of_users += 1;
	    print_str(11, 1, "existing box found         ");
	    return i;
	}
    }
//    enter_critical();

    lock_acquire(&MessageBoxen[empty_box].access_lock);


    MessageBoxen[empty_box].unused = 0;
    MessageBoxen[empty_box].number_of_users = 1;


    strcpy(MessageBoxen[empty_box].name, name);
    
    
    // add this box to current_running's box list
    current_running->boxes[empty_box] = 1;
    print_str(11, 1, "an unused box found     ");
    lock_release(&MessageBoxen[empty_box].access_lock);
//    print_str(11, 1, "leaving openning    ");
//    leave_critical();
    return i;
}

/* Closes a message box
 */
void do_mbox_close(mbox_t mbox)
{
    (void)mbox;
    /* TODO */
    if(mbox >= MAX_MBOXEN || mbox < 0)
	return;
    current_running->boxes[mbox] = 0;
    lock_acquire(&MessageBoxen[mbox].access_lock);
    if(--MessageBoxen[mbox].number_of_users == 0){
	MessageBoxen[mbox].unused = 1;
	MessageBoxen[mbox].name[0] = '\0';
	MessageBoxen[mbox].empty = 1;
    }
    lock_release(&MessageBoxen[mbox].access_lock);
}

/* Determine if the given
 * message box is full.
 * Equivalently, determine
 * if sending to this mbox
 * would cause a process
 * to block.
 */
int do_mbox_is_full(mbox_t mbox)
{
    (void)mbox;
    /* TODO */
    if(mbox >= MAX_MBOXEN || mbox < 0)
	ASSERT(0);
    return !(MessageBoxen[mbox].read_letter + MAX_MBOX_LENGTH == MessageBoxen[mbox].write_letter);
}

/* Enqueues a message onto
 * a message box.  If the
 * message box is full, the
 * process will block until
 * it can add the item.
 * You may assume that the
 * message box ID has been
 * properly opened before this
 * call.
 * The message is 'nbytes' bytes
 * starting at offset 'msg'
 */
void do_mbox_send(mbox_t mbox, void *msg, int nbytes)
{
    (void)mbox;
    (void)msg;
    (void)nbytes;

    /* TODO */
    if(mbox >= MAX_MBOXEN || mbox < 0)
	return;

    if(nbytes < 0 || nbytes > MAX_MESSAGE_LENGTH )
	return;
    enter_critical();
    if(MessageBoxen[mbox].full)
	block(&MessageBoxen[mbox].write_wait_queue);
    leave_critical();


    print_hex(1, 1, current_running->entry_point);
    print_str(2, 1, "sending a letter");    
    
    lock_acquire(&MessageBoxen[mbox].access_lock);


    int i;
    for(i = 0; i < nbytes && i < MAX_MESSAGE_LENGTH; i++)
	MessageBoxen[mbox].letters[MessageBoxen[mbox].write_letter % MAX_MBOX_LENGTH].message[i] = ((char*)msg)[i];
    MessageBoxen[mbox].letters[MessageBoxen[mbox].write_letter % MAX_MBOX_LENGTH].message[i] = '\0';

    MessageBoxen[mbox].write_letter += 1;
    if(MessageBoxen[mbox].write_letter == MessageBoxen[mbox].read_letter + MAX_MBOX_LENGTH)
	MessageBoxen[mbox].full = 1;
    // once we arrive here, we must have written a letter
    // the mailbox can not be empty 
    MessageBoxen[mbox].empty = 0;  
    /*
      unblock one task from wati queue
     */

    lock_release(&MessageBoxen[mbox].access_lock);

    enter_critical();
    if(!is_empty(&MessageBoxen[mbox].read_wait_queue))
	unblock((pcb_t*)dequeue(&MessageBoxen[mbox].read_wait_queue));
    leave_critical();
}

/* Receives a message from the
 * specified message box.  If
 * empty, the process will block
 * until it can remove an item.
 * You may assume that the
 * message box has been properly
 * opened before this call.
 * The message is copied into
 * 'msg'.  No more than
 * 'nbytes' bytes will by copied
 * into this buffer; longer
 * messages will be truncated.
 */
void do_mbox_recv(mbox_t mbox, void *msg, int nbytes)
{
    (void)mbox;
    (void)msg;
    (void)nbytes;
    /* TODO */
        /* TODO */
    if(mbox >= MAX_MBOXEN || mbox < 0)
	return;

    if(nbytes < 0 || nbytes > MAX_MESSAGE_LENGTH )
	return;

    enter_critical();
    if(MessageBoxen[mbox].empty)
	block(&MessageBoxen[mbox].read_wait_queue);
    leave_critical();
    
    lock_acquire(&MessageBoxen[mbox].access_lock);

    int i;
    for(i = 0; i < nbytes && i < MAX_MESSAGE_LENGTH; i++)
	((char*)msg)[i] = MessageBoxen[mbox].letters[MessageBoxen[mbox].read_letter % MAX_MBOX_LENGTH].message[i];
    ((char*)msg)[i] = '\0';

    MessageBoxen[mbox].read_letter += 1;
    if(MessageBoxen[mbox].read_letter == MessageBoxen[mbox].write_letter)
	MessageBoxen[mbox].empty = 1;
    // when we arrive here, we must have taken one letter
    // the mailbox can not be full
    MessageBoxen[mbox].full = 0;
    /*
      unblock one task from wati queue
     */

    lock_release(&MessageBoxen[mbox].access_lock);
    
    enter_critical();
    if(!is_empty(&MessageBoxen[mbox].write_wait_queue))
	unblock((pcb_t*)dequeue(&MessageBoxen[mbox].write_wait_queue));
    leave_critical();
}

