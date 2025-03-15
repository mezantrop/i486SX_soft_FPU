# Addition with zero (3.5 + 0.0)

.section .data
num1_zero:   .tfloat 3.5
num2_zero:   .tfloat 0.0

.section .text
.global main
main:
    fldt  num1_zero
    faddl num2_zero

    subl $12, %esp
    fstpt (%esp)
    movl (%esp), %eax
    addl $12, %esp
    ret
