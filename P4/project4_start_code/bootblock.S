	.text
	.globl main

main:
	# check the offset of main
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop

	# read kernel
	lw $8, read
	lw $4, kernel
	li $5, 0x0200 
	lw $6, os_size
	jalr $8

	# jump to kernel
	lw $8, kernel_main
	jal $8

	j $ra


	.data
read: .word 0x8007b1a8
kernel: .word 0xa0800200
kernel_main: .word 0xa0800200
os_size: .word
