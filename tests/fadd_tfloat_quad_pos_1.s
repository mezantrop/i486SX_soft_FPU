.section .data
num1:	.tfloat 3.55
num2:   .quad 0x4002000000000000    # 2.25

.section .text
.global main
main:
	fldt	num1			# 3.55 -> ST(0)
	faddl	num2			# 2.25 + ST(0)

	subl	$12, %esp
	fstpt	(%esp)
	movl	(%esp), %eax
	addl	$12, %esp

	ret
