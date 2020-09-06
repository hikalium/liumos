.intel_syntax noprefix

// System V AMD64 ABI:
//   args: RDI, RSI, RDX, RCX, R8, R9
//   callee-saved: RBX, RBP, R12, R13, R14, R15
//   caller-saved: otherwise

.global exit

// https://uclibc.org/docs/psABI-x86_64.pdf
// Figure 3.9: Initial Process Stack
// rsp + 8 * i: argv[i]
// rsp: argc
.global entry
entry:
	mov rdi, [rsp]
	lea rsi, [rsp + 8]
	call main
	mov rdi, 0 // exit code 0
	jmp exit
