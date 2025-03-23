# Addition involving NaN

.section .data
nan1:
    	.word 0x7FFF
    	.quad 0xC000000000000000
num3:   .quad 0x3FF0000000000000  # 1.0 in IEEE 754 double format

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
