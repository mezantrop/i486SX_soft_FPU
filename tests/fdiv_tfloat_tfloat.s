	.section .data
num1:	.float 3.5			# First single-precision number (3.5)
num2:	.float 2.5			# Second single-precision number (2.5)

	.section .text
	.global main			# Define the entry point for the program
main:
	# Load num1 (32-bit floating point) into FPU stack (ST(0))
	fld		num1			# Load num1 (float) into FPU stack (ST(0))

	# Load num2 (32-bit floating point) into FPU stack (ST(1))
	fld		num2			# Load num2 (float) into FPU stack (ST(1))

	# Divide ST(0) by ST(1), result will be in ST(0)
	fdivp					# Divide ST(1) by ST(0), store result in ST(1), and pop the register stack

	# Store the result into memory
	subl	$8, %esp		# Allocate space for the result (4-byte result)
	fstps	(%esp)			# Store the result into memory (single-precision)
	movl	(%esp), %eax	# Move lower 4 bytes to the return register for debugging
	addl	$8, %esp		# Clean up stack

	# Exit the program
	movl	$1, %eax		# syscall number for exit
	xorl	%ebx, %ebx		# exit code 0
	int		$0x80			# make syscall
