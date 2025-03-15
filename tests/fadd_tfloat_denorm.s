# Addition of denormalized numbers

.section .data
denorm1:   .tfloat 1.0e-4950
denorm2:   .tfloat 2.0e-4950

.section .text
.global main
main:
    fldt  denorm1
    faddl denorm2

    subl $12, %esp
    fstpt (%esp)
    movl (%esp), %eax
    addl $12, %esp
    ret
