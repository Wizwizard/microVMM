.section .text

.code16
.globl _start

_start:
  mov $0x45, %edx
  in (%dx), %al
key_in:
  mov $0x44, %edx
  in (%dx), %al
  mov %al, %bl
ack_key:
  mov $0x0, %al
  mov $0x45, %edx
  out %al, (%dx)
echo:
  mov $0x42, %edx
  mov %bl, %al
  out %al, (%dx)
  jmp key_in
