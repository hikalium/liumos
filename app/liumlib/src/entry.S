.intel_syntax noprefix

// System V AMD64 ABI:
//   args: RDI, RSI, RDX, RCX, R8, R9
//   callee-saved: RBX, RBP, R12, R13, R14, R15
//   caller-saved: otherwise

// https://uclibc.org/docs/psABI-x86_64.pdf
// Figure 3.9: Initial Process Stack
// rsp + 8 * i: argv[i]
// rsp: argc
.global _start_rs
.global _start
_start:
	mov rdi, [rsp]
	lea rsi, [rsp + 8]
	jmp _start_rs
