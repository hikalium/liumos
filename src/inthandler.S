.intel_syntax noprefix

// SDM Vol.3: 6.14.2 64-Bit Mode Stack Frame
// In IA-32e mode, the RSP is aligned to a 16-byte boundary
// before pushing the stack frame

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
	sub rsp, 512 + 8
	fxsave [rsp]

	mov rdx, rsp 
	mov rbp, rsp
	and rsp, -16
	call IntHandler
	mov rsp, rbp

// This function is also called by sleep_handler
.global RestoreRegistersAndIRETQ
RestoreRegistersAndIRETQ:
	fxrstor [rsp]
	add rsp, 512 + 8
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
