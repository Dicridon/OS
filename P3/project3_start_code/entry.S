#include "asm.h"
#include "regs.h"
#include "cp0regdefs.h"

  /* Hardcoded values. Do not change these!! */
  /* Offset of the kernel context in a PCB */
  .equ  KERNEL, 8
  /* Offset of the user context in a PCB */
  .equ  USER, 164
  /* Offset of %esp in a context */
  .equ  STACK, TF_REG29
  /* Offset of field nested_count in a PCB */
  .equ  NESTED_COUNT, 320
  /* Offset of field status in a PCB */	
  .equ  PCB_STATUS, 336
	
.macro PUTCHAR ch label
	addi  k0,$0,10000
	\label:
	addi  k0,k0,-1
	bne k0,$0,\label

	addi  k0,$0,\ch
	la    k1,0xbfe48000
	sb    k0,0(k1)
.endm

.macro STI
  mfc0    k0, CP0_STATUS
  li      k1, (STATUS_CU0 | 0x1)
  or      k0, k1
  mtc0    k0, CP0_STATUS
.endm

.macro CLI
  mfc0    k0, CP0_STATUS
  li      k1, (STATUS_CU0 | 0x1)
  or      k0, k1
  xori    k0, 0x1
  mtc0    k0, CP0_STATUS
.endm

/* Do not change any of these macros! */
/* Save registers/flags to the specified offset in the current PCB */
.macro SAVE_CONTEXT offset
/* TODO: need add */
	// save registers except ra and sp
	la   k0, current_running
	lw   k0, 0(k0)
	addi k0, k0, \offset
	sw   zero, TF_REG0(k0)
	sw   AT, TF_REG1(k0)
	sw   v0, TF_REG2(k0)
	sw   v1, TF_REG3(k0)
	
	sw   a0, TF_REG4(k0)
	sw   a1, TF_REG5(k0)
	sw   a2, TF_REG6(k0)
	sw   a3, TF_REG7(k0)

	sw   t0, TF_REG8(k0)
	sw   t1, TF_REG9(k0)
	sw   t2, TF_REG10(k0)
	sw   t3, TF_REG11(k0)
	sw   t4, TF_REG12(k0)
	sw   t5, TF_REG13(k0)
	sw   t6, TF_REG14(k0)
	sw   t7, TF_REG15(k0)
	
	sw   s0, TF_REG16(k0)
	sw   s1, TF_REG17(k0)
	sw   s2, TF_REG18(k0)
	sw   s3, TF_REG19(k0)
	sw   s4, TF_REG20(k0)
	sw   s5, TF_REG21(k0)
	sw   s6, TF_REG22(k0)
	sw   s7, TF_REG23(k0)
	
	sw   t8, TF_REG24(k0)
	sw   t9, TF_REG25(k0)
	sw   gp, TF_REG28(k0)
	sw   sp, TF_REG29(k0)
	sw   fp, TF_REG30(k0)
	sw   ra, TF_REG31(k0)
	
	mfc0 k1, CP0_STATUS  // Status
	sw   k1, TF_STATUS(k0)
	mfhi t1              // HI
	sw   t1, TF_HI(k0)
	mflo t1              // LO
	sw   t1, TF_LO(k0)
	mfc0 k1, CP0_BADVADDR
	sw   k1, TF_BADVADDR(k0)     // Badvaddr
	mfc0 k1, CP0_CAUSE
	sw   k1, TF_CAUSE(k0)     // Cause
	mfc0 k1, CP0_EPC
	sw   k1, TF_EPC(k0)     // EPC


	
/* TODO: end */
.endm

/* Restore registers/flags from the specified offset in the current PCB */
.macro RESTORE_CONTEXT offset
	/* TODO: need add */
	la   k0, current_running
	lw   k0, 0(k0)
	addi k0, k0, \offset

	lw   zero, TF_REG0(k0)
	lw   AT, TF_REG1(k0)
	lw   v0, TF_REG2(k0)
	lw   v1, TF_REG3(k0)

	lw   a0, TF_REG4(k0)
	lw   a1, TF_REG5(k0)
	lw   a2, TF_REG6(k0)
	lw   a3, TF_REG7(k0)
	
	lw   t0, TF_REG8(k0)
	lw   t1, TF_REG9(k0)
	lw   t2, TF_REG10(k0)
	lw   t3, TF_REG11(k0)
	lw   t4, TF_REG12(k0)
	lw   t5, TF_REG13(k0)
	lw   t6, TF_REG14(k0)
	lw   t7, TF_REG15(k0)
	lw   s0, TF_REG16(k0)
	lw   s1, TF_REG17(k0)
	lw   s2, TF_REG18(k0)
	lw   s3, TF_REG19(k0)
	lw   s4, TF_REG20(k0)
	lw   s5, TF_REG21(k0)
	lw   s6, TF_REG22(k0)
	lw   s7, TF_REG23(k0)
	lw   t8, TF_REG24(k0)
	lw   t9, TF_REG25(k0)
	
	lw   gp, TF_REG28(k0)
	
	lw   sp, TF_REG29(k0)
	lw   fp, TF_REG30(k0)
	lw   ra, TF_REG31(k0)
	
	lw   k1, TF_STATUS(k0)
	mtc0 k1, CP0_STATUS  // Status
	lw   k1, TF_HI(k0)
	mthi k1              // HI
	lw   k1, TF_LO(k0)
	mtlo k1              // LO
	lw   k1, TF_BADVADDR(k0)     // Badvaddr
	mtc0 k1, CP0_BADVADDR
	lw   k1, TF_CAUSE(k0)     // Cause
	mtc0 k1, CP0_CAUSE
	lw   k1, TF_EPC(k0)     // EPC
	mtc0 k1, CP0_EPC
/* TODO: end */
.endm

/* Increment disable_count, panicking on a double ENTER
 * Interrupts should be disabled before this macro is used
 */
.macro ENTER_CRITICAL
  la      k0, disable_count
  lw      k1, 0(k0)
  bne     k1, $0, panice
  nop
  addi    k1, k1, 1
  sw      k1, 0(k0)
.endm

/* Decrement disable_count, panicking on a double LEAVE
 * Interrupts should be enabled after this macro is used
 */
.macro LEAVE_CRITICAL
  la      k0, disable_count
  lw      k1, 0(k0)
  addi    k1, k1, -1
  bne     k1, $0, panicl
  nop
  sw      k1, 0(k0)
.endm

/* TODO: Test current_running->nested_count, clobbering only eflags */
.macro TEST_NESTED_COUNT
	
.endm

/** TODO end */

  .data
// Temporary storage for 32-bit registers
scratch:
  .long    0
// Temporary storage for 32-bit registers
scratch2:
  .long    0
// Temporary storage for syscall return value
return_value:
  .long    0
// Signals status of interrupts
    .globl    disable_count
disable_count:
  .long    0

  .text

NESTED(scheduler_entry,0,ra)
	/* TODO: need add */
	SAVE_CONTEXT(KERNEL)
	jal scheduler
	nop
	LEAVE_CRITICAL
	RESTORE_CONTEXT(KERNEL)
	STI
	jr ra
	nop
	/* TODO: end */
END(scheduler_entry)

LEAF(asm_start)
  /* Disable interrupts */
  mtc0    zero, CP0_STATUS

  /* Disable watch exception. */
  mtc0    zero, CP0_WATCHLO
  mtc0    zero, CP0_WATCHHI

  /* disable kernel mode cache */
  mfc0    t0, CP0_CONFIG
  and     t0, ~0x7
  ori     t0, 0x2
  mtc0    t0, CP0_CONFIG

  jr  ra
END(asm_start)

LEAF(reset_timer)
  mtc0 $0, CP0_COUNT
  mtc0 a0, CP0_COMPARE
  jr ra
END(reset_timer)

  .global exception_handler_start
  .global exception_handler_end
NESTED(general_exception_handler,0,sp)
exception_handler_start:
  .set  noat
  .set  mips32

  CLI

  mfc0  k0, CP0_CAUSE
  andi  k0, k0, CAUSE_EXCCODE   /* k0 = {exc_code,00} */
  la    k1, interrupt_handlers
  add   k0, k0, k1
  lw    k0, 0(k0)
  jr  k0     /* interrupt_handlers[exc_code](); */
exception_handler_end:
END(general_exception_handler)
  .set at
  .set reorder
  .text

LEAF(return_from_exception)
	STI

	eret
END(return_from_exception)

NESTED(simple_handler,0,sp)
  SAVE_CONTEXT(KERNEL)

  mfc0  k0, CP0_CAUSE           /* Read Cause register for IP bits */
  //mfc0  k1, CP0_STATUS        /* and Status register for IM bits */
  andi  k0, k0, CAUSE_IPL       /* Keep only IP bits from Cause */
  //and   k0, k0, k1            /* and mask with IM bits */
  clz   k0, k0                  /* Find first bit set, IP7..IP0; k0 = 16..23 */
  xori  k0, k0, 0x17            /* 16..23 => 7..0 */
  add   a0, k0, $0

  mfc0  k0, CP0_CAUSE
  andi  k0, k0, CAUSE_EXCCODE   /* k0 = {exc_code,00} */
  srl   k0, k0, 2
  add   a1, k0, $0

  jal   c_simple_handler

  mfc0  k0, CP0_CAUSE           /* Read Cause register for IP bits */
  andi  k1, k0, CAUSE_IPL       /* Keep only IP bits from Cause */
  xor   k0, k0, k1
  mtc0  k0, CP0_CAUSE

  LEAVE_CRITICAL
  RESTORE_CONTEXT(KERNEL)
  j return_from_exception
END(simple_handler)

NESTED(handle_syscall,0,sp)
  SAVE_CONTEXT(USER)
  //RESTORE_CONTEXT(KERNEL)
  add   a3,a2,$0
  add   a2,a1,$0
  add   a1,a0,$0
  add   a0,v0,$0

  jal system_call_helper

  LEAVE_CRITICAL
  RESTORE_CONTEXT(USER)
  j return_from_exception
END(handle_syscall)

NESTED(handle_int,0,sp)
  /* TODO: timer_irq */
	/* read int IP and handle clock interrupt or just call do_nothing */
	SAVE_CONTEXT(USER)
	mfc0  k0, CP0_CAUSE
	andi  k0, k0, CAUSE_IPL
	andi  k0, k0, 0x8000
	bnez  k0, time 
	nop
	j     2f 
	nop
time:
	mtc0    zero, CP0_COUNT
	li      k1, 150000000
	mtc0    k1, CP0_COMPARE
	
	jal     time_irq
	nop

2:

	mfc0 k0, CP0_CAUSE
	lui  t0, 0xffff
	ori  t0, t0, 0x00ff
	and  k0, k0, t0
	mtc0 k0, CP0_CAUSE

3:

	nop
	RESTORE_CONTEXT(USER)
	STI
	eret



  /* TODO:end */
END(handle_int)

ret:
  LEAVE_CRITICAL   // del disable_count
  eret

LEAF(get_cp0_status)
  mfc0  v0, CP0_STATUS
  jr ra
END(get_cp0_status)

LEAF(set_cp0_status)
  mtc0  a0, CP0_STATUS
  jr ra
END(set_cp0_status)


LEAF(enter_critical)
  CLI
  ENTER_CRITICAL
  jr ra
END(enter_critical)

LEAF(leave_critical)
  LEAVE_CRITICAL
  STI
  jr ra
END(leave_critical)

/* Error Message: Write "PANIC" to the screen and enter an infinite loop */
LEAF(panic)
  CLI
  PUTCHAR 0x50 P1// 'P'
  j panic
END(panic)
LEAF(panice)
  CLI
  PUTCHAR 69 P2// 'E'
  j panic
END(panice)
LEAF(panicl)
  CLI
  PUTCHAR 76 P3// 'L'
  j panic
END(panicl)
//  cli
//  movl    $0xb800a, %esp
//  pushw    $0x4f43
//  pushw    $0x4f49
//  pushw    $0x4f4e
//  pushw    $0x4f41
//  pushw    $0x4f50
LEAF(infinite_loop)
//        hlt
  j    infinite_loop
END(infinite_loop)
