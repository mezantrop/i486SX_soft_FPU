.section .data
num1:	.tfloat -3.5
num2:	.quad 0x4004000000000000	# 2.5 in IEEE 754 (double precision)

.section .text
.global main
main:
	fldt	num1			# ST(0) = -3.5
	fldl	num2			# ST(0) = 2.5, ST(1) = -3.5
	fcompp					# Compare ST(0) with ST(1), pop both

	subl	$4, %esp
	fnstsw	(%esp)			# Store FPU status word
	movzwl	(%esp), %eax	# Move full 16-bit status to eax (zero-extended)
	addl	$4, %esp

	ret
