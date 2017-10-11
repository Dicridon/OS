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

	#need add code
	#read kernel
	
	li $4, 0xa0800200
	li $5, 0x200
	li $6, 0x110

	jal 0x8007b1a8
	jal 0xa080026c
