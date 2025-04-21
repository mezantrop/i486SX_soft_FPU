	.section .data
num1:	.float 3.5			# First float
num2:	.float 2.5			# Second float

	.section .text
	.global main

main:
	# Load numbers (32-bit floating point) into FPU stack
	fld		num1			# ST(0) = 3.5
	fld		num2			# ST(0) = 2.5, ST(1) = 3.5

	# Perform multiplication: ST(1) = ST(1) * ST(0), pop
	fmulp					# ST(1) = 3.5 * 2.5 = 8.75, ST(0) ← popped

	# Store the result into memory
	subl	$8, %esp		# Allocate space for the result (4-byte result)
	fstps	(%esp)			# Store the result into memory (single-precision)
	movl	(%esp), %eax	# Move lower 4 bytes to the return register for debugging
	addl	$8, %esp		# Clean up stack

	# Exit the program
	movl	$1, %eax		# syscall: exit
	xorl	%ebx, %ebx		# exit code 0
	int		$0x80			# syscall
