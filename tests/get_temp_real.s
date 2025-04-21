	.section .data
	.align 4

temp_real_value:
	.long 0x89abcdef		# significand low (DWORD a)
	.long 0x01234567		# significand high (DWORD b)
	.short 0x4000			# exponent (WORD)

	.section .text
	.globl main
main:
	fldt temp_real_value

	subl $12, %esp
	fstpt (%esp)
	movl (%esp), %eax
	addl $12, %esp

	movl $1, %eax			# syscall: exit
	xorl %ebx, %ebx
	int $0x80
