.section .data
num1:   .tfloat -3.7             # extended precision float (long double)

.section .text
.global main
main:
    fldt    num1                # ST(0) = -3.7
    frndint                     # rounding to -4.0

    subl    $12, %esp
    fstpt   (%esp)
    mov     (%esp), %eax
    addl    $12, %esp

    ret
