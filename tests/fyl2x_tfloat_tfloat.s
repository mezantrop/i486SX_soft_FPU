.section .data
val1:   .tfloat 10.1
val2:   .tfloat 2000.1

.section .text
.global main
main:
    fldt    val1            # ST(0) = 10.1
    fldt    val2            # ST(0) = 2000.1, ST(1) = 10.1

    fyl2x                   # ST(1) = ST(1) * log2(ST(0)); pop ST(0)

    subl    $12, %esp
    fstpt   (%esp)
    movl    (%esp), %eax
    addl    $12, %esp

    movl    $1, %eax        # syscall: exit
    xorl    %ebx, %ebx      # status 0
    int     $0x80
