.section .text

.code16
.globl _start

_start:
  mov $0x46, %edx
  out $0x05, (%dx)
key_in:
  mov $0x44, %edx
  in (%dx), %al
  mov %al, %bl
key_in_flag:
  mov $0x45, %edx
  in (%dx), %al
ack_key:
  mov $0x0, %al
  mov $0x45, %edx
  cmp (%dx), $0x0
  jz ack_timer
  out %al, (%dx)
write:
  mov $0x42, %edx
  mov %bl, %al
  out %al, (%dx)
enable_timer:
  mov $0x47, %edx
  in (%dx), $al
ack_timer:
  mov $0x47, $edx
  mov $0x0, %al
  out %al, (%dx)
  jmp key_in


