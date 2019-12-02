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

/* This macro avoids symbol mismatching in test binary on macOS */
#if defined(__APPLE__)
# define cdecl(s) _##s
#else
# define cdecl(s) s
#endif

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

.global ReadIOPort32
ReadIOPort32:
  mov edx, ecx
  xor rax, rax
  in eax, dx
  ret

.global WriteIOPort32
WriteIOPort32:
	mov eax, edx
  mov dx, cx
  out dx, eax
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

.global cdecl(RepeatMoveBytes)
cdecl(RepeatMoveBytes):
	// rcx: count
	// rdx: dst
	// r8: src
	mov rax, [rsp]
	xchg rdi, rdx
	xchg rsi, r8
	rep movsb
	xchg rdi, rdx
	xchg rsi, r8
	ret

.global cdecl(RepeatMove4Bytes)
cdecl(RepeatMove4Bytes):
	// rcx: count
	// rdx: dst
	// r8: src
	mov rax, [rsp]
	xchg rdi, rdx
	xchg rsi, r8
	rep movsd
	xchg rdi, rdx
	xchg rsi, r8
	ret

.global cdecl(RepeatStore4Bytes)
cdecl(RepeatStore4Bytes):
	// rcx: count
	// rdx: dst
	// r8: value
	mov rax, [rsp]
	xchg rdi, rdx
	mov rax, r8
	rep stosd
	xchg rdi, rdx
	ret

.global cdecl(RepeatMove8Bytes)
cdecl(RepeatMove8Bytes):
	// rcx: count
	// rdx: dst
	// r8: src
	mov rax, [rsp]
	xchg rdi, rdx
	xchg rsi, r8
	rep movsq
	xchg rdi, rdx
	xchg rsi, r8
	ret

.global cdecl(RepeatStore8Bytes)
cdecl(RepeatStore8Bytes):
	// rcx: count
	// rdx: dst
	// r8: value
	mov rax, [rsp]
	xchg rdi, rdx
	mov rax, r8
	rep stosq
	xchg rdi, rdx
	ret

.global CLFlush
CLFlush:
	clflush [rcx]
	ret

.global CLFlushOptimized
CLFlushOptimized:
	clflushopt [rcx]
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
