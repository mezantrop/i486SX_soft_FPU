# Addition involving NaN

.section .data
nan1:
    .word 0x7FFF
    .quad 0xC000000000000000
num3:   .tfloat 1.0

.section .text
.global main
main:
    fldt  nan1
    faddl num3

    subl $12, %esp
    fstpt (%esp)
    movl (%esp), %eax
    addl $12, %esp
    ret
