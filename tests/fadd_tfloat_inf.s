# Addition involving infinity

.section .data
pos_inf:	.tfloat 1.0e+4932
neg_inf:	.tfloat -1.0e+4932

.section .text
.global main
main:
	fldt	pos_inf
	faddl	neg_inf

	subl	$12, %esp
	fstpt	(%esp)
	movl	(%esp), %eax
	addl	$12, %esp
	ret
