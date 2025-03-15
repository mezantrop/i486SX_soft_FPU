# Perform addition of two extended precision (80-bit) floating point numbers

.section .data
num1:   .tfloat 3.5
num2:   .tfloat 2.5

.section .text
.global main
main:
    fldt  num1         # FPU OP1. Load num1 into ST(0)
    faddl num2         # FPU OP2. Add num2 to ST(0)

    subl $12, %esp     # Allocate space for 80-bit (10-byte) return value
    fstpt (%esp)       # FPU OP3. Store result as extended precision (80-bit)
    movl (%esp), %eax  # Move lower 4 bytes to return register (for debugging)
    addl $12, %esp     # Clean up stack

    ret                # Return
