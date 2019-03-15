.intel_syntax noprefix

// Microsoft x64 calling convention:
//   args: rcx, rdx, r8, r9
//   callee-saved: RBX, RBP, RDI, RSI, RSP, R12, R13, R14, R15
//   caller-saved: RAX, RCX, RDX, R8, R9, R10, R11
// System V AMD64 ABI:
//   args: RDI, RSI, RDX, RCX, R8, R9
//   callee-saved: RBX, RBP, R12, R13, R14, R15
//   caller-saved: otherwise
// Linux kernel x86-64 syscall:
//   idx: RAX
//   args: RDI, RSI, RDX, R10, R8, R9

// Following functions are written with ms_abi calling convention.
// __attribute__((ms_abi)) will be required when used from C code.

.global Die
Die:
	cli
	hlt
	jmp Die

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

.global ReadCR2
ReadCR2:
	mov rax, cr2
	ret

.global ReadCR3
ReadCR3:
	mov rax, cr3
	ret

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

.global AsmIntHandler00_DivideError
AsmIntHandler00_DivideError:
	push 0
	push rcx
	mov rcx, 0x00
	jmp IntHandlerWrapper

.global AsmIntHandler03
AsmIntHandler03:
	push 0
	push rcx
	mov rcx, 0x03
	jmp IntHandlerWrapper

.global AsmIntHandler06
AsmIntHandler06:
	push 0
	push rcx
	mov rcx, 0x06
	jmp IntHandlerWrapper

.global AsmIntHandler07_DeviceNotAvailable
AsmIntHandler07_DeviceNotAvailable:
	push 0
	push rcx
	mov rcx, 0x07
	jmp IntHandlerWrapper

.global AsmIntHandler08
AsmIntHandler08:
	push rcx
	mov rcx, 0x08
	jmp IntHandlerWrapper

.global AsmIntHandler0D
AsmIntHandler0D:
	push rcx
	mov rcx, 0x0D
	jmp IntHandlerWrapper

.global AsmIntHandler0E
AsmIntHandler0E:
	push rcx
	mov rcx, 0x0E
	jmp IntHandlerWrapper

.global AsmIntHandler10_x87FPUError
AsmIntHandler10_x87FPUError:
	push 0
	push rcx
	mov rcx, 0x10
	jmp IntHandlerWrapper

.global AsmIntHandler13_SIMDFPException
AsmIntHandler13_SIMDFPException:
	push 0
	push rcx
	mov rcx, 0x13
	jmp IntHandlerWrapper

.global AsmIntHandler20
AsmIntHandler20:
	push 0
	push rcx
	mov rcx, 0x20
	jmp IntHandlerWrapper

.global AsmIntHandler21
AsmIntHandler21:
	push 0
	push rcx
	mov rcx, 0x21
	jmp IntHandlerWrapper

.global AsmIntHandlerNotImplemented
AsmIntHandlerNotImplemented:
	push 0
	push rcx
	mov rcx, 0x100
	jmp IntHandlerWrapper

.global IntHandler
// TODO: Save xmm registers
IntHandlerWrapper:
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

	mov rdx, rsp 
	mov rbp, rsp
	and rsp, -16
	call IntHandler
	mov rsp, rbp

  pop rax
  pop rdx
  pop rbx
  pop rbp
  pop rsi
  pop rdi
  pop r8
  pop r9
  pop r10
  pop r11
  pop r12
  pop r13
  pop r14
  pop r15

	pop rcx
	add rsp, 8
	iretq

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

.global ReadRSP
ReadRSP:
 	mov rax, rsp
	ret

.global ChangeRSP
ChangeRSP:
	mov rax, [rsp]
	mov rsp, rcx
	push rax
	ret

.global JumpToKernel
JumpToKernel:
	// rcx: kernel ptr
	// rdx: kernel info
	// r8: kernel stack
	// convert calling convention from Microsoft x64 to SystemV AMD64
	push rbp
	mov rbp, rsp
	mov rsp, r8

	mov rdi, rdx
	call rcx

	mov rsp, rbp
	pop rbp
	ret

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

