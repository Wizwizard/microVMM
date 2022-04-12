.section .text

.code16
.globl _start

_start:
    mov $0x42, %dx

    loop:
        mov $0x44, %ah
        in %ah,%dx
        jmp loop
