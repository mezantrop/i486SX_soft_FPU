.section .data
num1:	.tfloat 3.5			# long double (80-bit)
num2:	.tfloat -2.5		# negative long double (80-bit)

	.section .text
	.global main
main:
	fldt	num1
	fldt	num2
	fmulp					# ST(1) = ST(1) * ST(0); pop ST(0)

	subl	$12, %esp
	fstpt	(%esp)
	movl	(%esp), %eax
	addl	$12, %esp

	movl	$1, %eax
	xorl	%ebx, %ebx
	int		$0x80
