#
# Perform addition of two single-precision float numbers
#

.section .data
num1:   .float 3.5
num2:   .float 2.5

.section .text
.global main
main:
    fld  num1          # FPU OP #1. Load num1 into ST(0)
    fadd num2          # #2. Add num2 to ST(0)
    
    subl $8, %esp      # Allocate space for float return value
    fstps (%esp)       # #3. Store result to stack as single-precision float
    movl (%esp), %eax  # Move result to return register
    addl $8, %esp      # Clean up stack
    
    ret                # Return

