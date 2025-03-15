# Addition of a positive and a negative number (-3.5 + 2.5)

.section .data
num1_neg:   .tfloat -3.5
num2_pos:   .tfloat 2.5

.section .text
.global main
main:
    fldt  num1_neg     # Load num1 into ST(0)
    faddl num2_pos     # Add num2 to ST(0)

    subl $12, %esp
    fstpt (%esp)
    movl (%esp), %eax
    addl $12, %esp
    ret
