.section .data
val:	.tfloat 2.25			# sqrt(2.25) = 1.5

.section .text
.global main
main:
	fldt	val					# ST(0) = 2.25
	fsqrt						# ST(0) = sqrt(2.25) = 1.5

	subl	$12, %esp
	fstpt	(%esp)
	mov		(%esp), %eax
	addl	$12, %esp

	ret
