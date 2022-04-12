.section .text

.code16
.globl _start
_start:
    loop:
        in $0x60, %ax
        cmp %ax, %bx
        jz loop
        mov $0x42, %dx
        out %ax, (%dx)
        hlt
        jmp loop

