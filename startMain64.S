.globl __main

__main:
	call smain
	movl $1, %eax
	xorl %ebx, %ebx
	int $0x80
