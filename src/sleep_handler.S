.intel_syntax noprefix

.global Sleep
Sleep:
	// Align stack on 16-byte boundary
	push rbp
	mov rbp, rsp
	and rsp, -16

	// Build interrupt stack frame
	// CPU: SS, RSP, RFLAGS, CS, RIP
	mov rax, ss
	push rax
	push rsp
	add qword ptr [rsp], 8
	pushfq
	mov rax, cs
	push rax
	call SleepSub

	mov rsp, rbp
	pop rbp
	ret

.global SleepHandler
.global RestoreRegistersAndIRETQ
SleepSub:
	push 0
	push rcx
  push r15
  push r14
  push r13
  push r12
  push r11
  push r10
  push r9
  push r8
  push rdi
  push rsi
  push rbp
  push rbx
  push rdx
  push rax
	sub rsp, 512 + 8
	fxsave64[rsp]

	cli

	mov rdx, rsp 
	mov rbp, rsp
	and rsp, -16
	call SleepHandler
	mov rsp, rbp

	jmp RestoreRegistersAndIRETQ

