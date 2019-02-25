.intel_syntax noprefix

/*
UEFI 2.3.4.2 Detailed Calling Conventions
The caller passes the first four integer arguments in registers.
...left to right in Rcx, Rdx, R8, and R9 

The registers Rax, Rcx Rdx R8, R9, R10, R11, and XMM0-XMM5 are volatile
The registers RBX, RBP, RDI, RSI, R12, R13, R14, R15, and XMM6-XMM15 should be preserved.
*/

.global ReadCPUID
ReadCPUID:
	mov r9, rcx
	mov eax, edx
	mov ecx, r8d
	cpuid
	mov [r9], eax
	mov [r9 + 4], ebx
	mov [r9 + 8], ecx
	mov [r9 + 12], edx
	ret

.global ReadMSR
ReadMSR:
	mov rax, 0
	rdmsr
	shl rdx, 32
	or	rax, rdx
	ret

.global WriteMSR
WriteMSR: // WriteMSR(rcx: msr_index, rdx: data)
	mov rax, rdx
	shr rdx, 32
	wrmsr
	ret

.global ReadGDTR
ReadGDTR:
	sgdt [rcx]
	ret

.global WriteGDTR
WriteGDTR:
	lgdt [rcx]
	ret

.global ReadIDTR
ReadIDTR:
	sidt [rcx]
	ret

.global WriteIDTR
WriteIDTR:
	lidt [rcx]
	ret

.global WriteTaskRegister
WriteTaskRegister:
	ltr cx
	ret

.global Int03
Int03:
	int3
	ret

.global ReadIOPort8
ReadIOPort8:
  mov edx, ecx
  xor rax, rax
  in al, dx
  ret

.global WriteIOPort8
WriteIOPort8:
	mov al, dl
  mov dx, cx
  out dx, al
  ret

.global StoreIntFlag
StoreIntFlag:
	sti
	ret

.global StoreIntFlagAndHalt
StoreIntFlagAndHalt:
	sti
	hlt
	ret

.global ClearIntFlag
ClearIntFlag:
	cli
	ret

.global Die
Die:
	cli
	hlt
	jmp Die

.global ReadCSSelector
ReadCSSelector:
	mov rax, 0
	mov ax, cs
	ret

.global ReadSSSelector
ReadSSSelector:
	mov rax, 0
	mov ax, ss
	ret

.global WriteCSSelector
WriteCSSelector:
	# The MOV instruction CANNOT be used to load the CS register
	# use far-jump instead.
	lea rax, [rip + LWriteCSSelector_end]
	push cx
	push rax
	ljmp [rsp]
LWriteCSSelector_end:
	add rsp, 8 + 2
	ret

.global WriteSSSelector
WriteSSSelector:
	mov ss, cx
	ret

.global WriteDataAndExtraSegmentSelectors
WriteDataAndExtraSegmentSelectors:
	mov ds, cx
	mov es, cx
	mov fs, cx
	mov gs, cx
	ret

.global ReadCR2
ReadCR2:
	mov rax, cr2
	ret

.global ReadCR3
ReadCR3:
	mov rax, cr3
	ret

.global WriteCR3
WriteCR3:
	mov cr3, rcx
	ret

.global CompareAndSwap
CompareAndSwap:
	// rcx: target addr
	// rdx: expected value
	// r8: new value
	mov rax, r8
	cmpxchg [rcx], r8
	ret

.global SwapGS
SwapGS:
	swapgs
	ret


ContextSwitch:
	// r8: InterruptInfo*
	// rax: ContextSwitchRequest*
	mov rcx, [rax]	// Context *from
	mov rdx, [rax + 8]	// Context *to
	// save InterruptInfo(rip, cs, eflags, rsp, ss)
	mov rax, [r8]
	mov [rcx], rax
	mov rax, [r8 + 8]
	mov [rcx + 8], rax
	mov rax, [r8 + 16]
	mov [rcx + 16], rax
	mov rax, [r8 + 24]
	mov [rcx + 24], rax
	mov rax, [r8 + 32]
	mov [rcx + 32], rax
	// save scratch registers (already pushed on stack)
	pop qword ptr [rcx + 40]
	pop qword ptr [rcx + 48]
	pop qword ptr [rcx + 56]
	pop qword ptr [rcx + 64]
	pop qword ptr [rcx + 72]
	pop qword ptr [rcx + 80]
	pop qword ptr [rcx + 88]
	// save rest of general registers (alive on registers)
	mov [rcx + 96], rbx
	mov [rcx + 104], rbp
	mov [rcx + 112], rsi
	mov [rcx + 120], rdi
	mov [rcx + 128], r12
	mov [rcx + 136], r13
	mov [rcx + 144], r14
	mov [rcx + 152], r15
	// restore InterruptInfo to interrupt stack(rip, cs, eflags, rsp, ss)
	mov rax, [rdx]
	mov [r8], rax
	mov rax, [rdx + 8]
	mov [r8 + 8], rax
	mov rax, [rdx + 16]
	mov [r8 + 16], rax
	mov rax, [rdx + 24]
	mov [r8 + 24], rax
	mov rax, [rdx + 32]
	mov [r8 + 32], rax
	// restore scratch registers to stack
	push qword ptr [rdx + 88]
	push qword ptr [rdx + 80]
	push qword ptr [rdx + 72]
	push qword ptr [rdx + 64]
	push qword ptr [rdx + 56]
	push qword ptr [rdx + 48]
	push qword ptr [rdx + 40]
	// save rest of general registers (alive on registers)
	mov rbx, [rdx + 96]
	mov rbp, [rdx + 104]
	mov rsi, [rdx + 112]
	mov rdi, [rdx + 120]
	mov r12, [rdx + 128]
	mov r13, [rdx + 136]
	mov r14, [rdx + 144]
	mov r15, [rdx + 152]
	// switch cr3 if necessary
	mov rax, [rdx + 160]
	cmp rax, [rcx + 160]
	je RestoreScratchRegistersAndIRET
	mov cr3, rax
	jmp RestoreScratchRegistersAndIRET

.global IntHandler
// The registers Rax, Rcx, Rdx, R8, R9, R10, R11, and XMM0-XMM5 are volatile
// TODO: Save xmm registers
IntHandlerWrapper:
	push rax
	push r9
	push r10
	push r11

	push r8
	swapgs
	call IntHandler
	swapgs
	pop r8

	cmp rax, 0
	jnz	ContextSwitch
RestoreScratchRegistersAndIRET:
	pop r11
	pop r10
	pop r9
	pop rax
	pop r8
	pop rdx
	pop rcx
	iretq

.global AsmIntHandler03
AsmIntHandler03:
	push rcx
	push rdx
	push r8
	mov rcx, 0x03
	lea r8, [rsp + 8 * 3]
	jmp IntHandlerWrapper

.global AsmIntHandler0D
AsmIntHandler0D:
	push rcx
	push rdx
	push r8
	mov rcx, 0x0D
	mov rdx, [rsp + 8 * 3] // error_code
	lea r8, [rsp + 8 * 4]
	jmp IntHandlerWrapper

.global AsmIntHandler0E
AsmIntHandler0E:
	push rcx
	push rdx
	push r8
	mov rcx, 0x0E
	mov rdx, [rsp + 8 * 3] // error_code
	lea r8, [rsp + 8 * 4]
	jmp IntHandlerWrapper

.global AsmIntHandler20
AsmIntHandler20:
	push rcx
	push rdx
	push r8
	mov rcx, 0x20
	lea r8, [rsp + 8 * 3]
	jmp IntHandlerWrapper

.global AsmIntHandler21
AsmIntHandler21:
	push rcx
	push rdx
	push r8
	mov rcx, 0x21
	lea r8, [rsp + 8 * 3]
	jmp IntHandlerWrapper

.global AsmIntHandlerNotImplemented
AsmIntHandlerNotImplemented:
	push rcx
	push rdx
	push r8
	mov rcx, 0x100
	lea r8, [rsp + 8 * 3]
	jmp IntHandlerWrapper

.global Disable8259PIC
Disable8259PIC:
	mov al, 0xff
	out 0xa1, al
	out 0x21, al
	ret

// https://docs.microsoft.com/en-us/windows/desktop/devnotes/-win32-chkstk
.global __chkstk
__chkstk:
	// do nothing
	ret

