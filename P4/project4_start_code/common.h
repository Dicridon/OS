/* common.h */
/* Do not change this file */

#ifndef COMMON_H
#define COMMON_H

#define NULL ((void *) 0)

/* Extra Credit */
#define EC_PRIORITIES
#define EC_DEADLOCK

/* System calls
 * This enumeration gives every system call
 * a unique syscall number.
 * They are exposed to processes in syslib.h, eg yield()
 * They are exposed to kernel/threads in scheduler.h, eg do_yield()
 */
typedef enum {
    SYSCALL_YIELD=0,         // 0
    SYSCALL_EXIT,            // 1
    SYSCALL_GETPID,          // 2
    SYSCALL_GETPRIORITY,     // 3
    SYSCALL_SETPRIORITY,     // 4
    SYSCALL_SLEEP,           // 5
    SYSCALL_SHUTDOWN,        // 6
    SYSCALL_WRITE_SERIAL,    // 7
    SYSCALL_PRINT_CHAR,      // 8
    SYSCALL_SPAWN,           // 9
    SYSCALL_KILL,            // 10
    SYSCALL_WAIT,            // 11
    SYSCALL_MBOX_OPEN,       // 12
    SYSCALL_MBOX_CLOSE,      // 13
    SYSCALL_MBOX_SEND,       // 14
    SYSCALL_MBOX_RECV,       // 15
    SYSCALL_TIMER,           // 16
    SYSCALL_LOCK_INIT,       // 18
    SYSCALL_LOCK_ACQUIRE,    // 19
    SYSCALL_LOCK_RELEASE,    // 20
    NUM_SYSCALLS             // 21
} syscall_t;

/* Assertion macros */
#define ASSERT2(p, s) \
do { \
    if (!(p)) { \
         print_str(0, 0, "Assertion failure: ");  \
         print_str(0, 19, s);                     \
         print_str(1, 0, "file: ");               \
         print_str(1, 6, __FILE__);               \
         print_str(2, 0, "line: ");               \
         print_int(2, 6, __LINE__);               \
        while (1); \
    } \
} while (0)
#define ASSERT(p) ASSERT2(p, #p)
#define HALT(s) ASSERT2(FALSE, s)

/* Commonly used types */
typedef enum {
    FALSE, TRUE
} bool_t;

typedef signed char int8_t;
typedef short int int16_t;
typedef long int32_t;
typedef long long int64_t;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
typedef unsigned long long uint64_t;

typedef int pid_t;
typedef int mbox_t;

/* A static upper limit on the
 * number of message boxes
 * at any time.
 */
#define MAX_MBOXEN          (32)

/* The longest name of a message
 * box.
 */
#define MBOX_NAME_LENGTH    (32)

/* A static upper limit on the
 * length of any one mbox.
 */
#define MAX_MBOX_LENGTH     (32)

/* The maximum length of any
 * message, it bytes
 */
#define MAX_MESSAGE_LENGTH  (32)

#endif                          /* COMMON_H */
