# Perform division of two single-precision (32-bit) floating point numbers

.section .data
num1:	.float 3.5			# First float number (numerator)
num2:	.float 2.5			# Second float number (denominator)

.section .text
.global main
main:
	fld		num1			# FPU OP #1. Load num1 into ST(0)
	fdiv	num2			# #2. Divide ST(0) by num2 (ST(0) = num1 / num2)

	subl	$8, %esp		# Allocate space for float return value
	fstps	(%esp)			# #3. Store result to stack as single-precision float
	movl	(%esp), %eax	# Move result to return register
	addl	$8, %esp		# Clean up stack

	ret
