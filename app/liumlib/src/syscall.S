// liumOS supports a subset of x86_64 Linux syscalls.
// https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86_64-64_bit

// System V AMD64 ABI:
//   args: RDI, RSI, RDX, RCX, R8, R9
//   callee-saved: RBX, RBP, R12, R13, R14, R15
//   caller-saved: otherwise

// Linux kernel x86-64 syscall:
//   idx: RAX
//   args: RDI, RSI, RDX, R10, R8, R9

.intel_syntax noprefix

.global sys_read
sys_read:
	mov rax, 0
	syscall
	ret

.global sys_write
sys_write:
	mov rax, 1
	syscall
	ret

.global sys_open
sys_open:
	mov rax, 2
	syscall
	ret

.global sys_exit
sys_exit:
	mov rax, 60
	syscall

.global sys_getdents64
sys_getdents64:
	mov rax, 0xd9
	syscall
  ret
