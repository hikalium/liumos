.intel_syntax noprefix

// Linux kernel x86-64 syscall:
//   idx: RAX
//   args: RDI, RSI, RDX, R10, R8, R9
.global AsmSyscallHandler
AsmSyscallHandler:
	push rcx	// Saved RIP
	push r11	// Saved RFLAGS

	push rbx
	push rbp
	push r12
	push r13
	push r14
	push r15

	push r9
	push r8
	push r10
	push rdx
	push rsi
	push rdi
	push rax

	mov rcx, rsp
	mov rbp, rsp
	and rsp, -16
	call SyscallHandler
	mov rsp, rbp

	pop rax
	pop rdi
	pop rsi
	pop rdx
	pop r10
	pop r8
	pop r9

	pop r15
	pop r14
	pop r13
	pop r12
	pop rbp
	pop rbx

	pop r11	// Saved RFLAGS
	pop rcx	// Saved RIP
	sysretq

