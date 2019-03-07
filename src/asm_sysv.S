.intel_syntax noprefix

// System V AMD64 ABI:
//   args: RDI, RSI, RDX, RCX, R8, R9
//   callee-saved: RBX, RBP, R12, R13, R14, R15
//   caller-saved: otherwise

// rcx -> rdi
// rdx -> rsi
// r8 -> rdx
// r9 -> rcx
