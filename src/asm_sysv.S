.intel_syntax noprefix

// System V AMD64 ABI:
//   args: RDI, RSI, RDX, RCX, R8, R9
//   callee-saved: RBX, RBP, R12, R13, R14, R15
//   caller-saved: otherwise

.global ReadIOPort8
ReadIOPort8:
  mov edx, edi
  xor rax, rax
  in al, dx
  ret

.global WriteIOPort8
WriteIOPort8:
	mov ax, si
  mov dx, di
  out dx, al
  ret
