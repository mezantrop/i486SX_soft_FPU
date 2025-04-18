.section .data
base:   .tfloat 2.5				# Value to scale
exp:    .tfloat 3.0				# scale

.section .text
.global main
main:
	fldt	exp					# ST(0) = 3.0
	fldt	base				# ST(0) = 2.5, ST(1) = 3.0
	fscale						# ST(0) = ST(0) * 2^ST(1) => 2.5 * 8 = 20.0

	subl	$12, %esp
	fstpt	(%esp)
	mov		(%esp), %eax
	addl	$12, %esp

	ret
