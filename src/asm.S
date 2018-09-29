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

.global ReadGDTR
ReadGDTR:
	sgdt [rcx]
	ret

.global ReadIDTR
ReadIDTR:
	sidt [rcx]
	ret

.global WriteIDTR
WriteIDTR:
	lidt [rcx]
	ret

.global Int03
Int03:
	int3
	ret

.global IntHandler03
.global AsmIntHandler03
AsmIntHandler03:
	call IntHandler03
  iretq

.global Disable8259PIC
Disable8259PIC:
	mov al, 0xff
	out 0xa1, al
	out 0x21, al
	ret
