.intel_syntax noprefix
.global read
read:
  mov rax, 0
  syscall
  ret

.global write
write:
  mov rax, 1
  syscall
  ret

.global exit
  exit:
  mov rax, 60
  syscall
