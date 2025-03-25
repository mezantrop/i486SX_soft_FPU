# Addition causing overflow

.section .data
big1:	.tfloat 1.0e+4932
big2:	.tfloat 1.0e+4932

.section .text
.global main
main:
	fldt	big1
	faddl	big2

	subl	$12, %esp
	fstpt	(%esp)
	movl	(%esp), %eax
	addl	$12, %esp
	ret
