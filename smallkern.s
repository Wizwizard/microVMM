.globl _start
    .code16

loop:
    in $0x60, %al
    cmp %al, %cl
    jz loop
    mov $0x42, %dx
    out %al, (%dx)
    hlt
    jmp loop

