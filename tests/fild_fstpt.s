.section .data
num_int:	.word -7				# 16-bit integer (short)

.section .text
.global main
main:
	fild	num_int					# FPU OP1. Load 16-bit integer into ST(0)

	subl	$12, %esp				# Allocate space for 80-bit (10-byte) return value
	fstpt	(%esp)					# FPU OP2. Store result as extended precision (80-bit)
	movl	(%esp), %eax			# Move lower 4 bytes to return register (for debugging)
	addl	$12, %esp				# Clean up stack
	ret
