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
  push rsp  // This pushes the value of RSP before executing this
  add qword ptr [rsp], 8  // Adjust saved RSP
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

// __attribute__((ms_abi)) bool CompareAndExchange64(uint64_t* dst,
//                                                   uint64_t expected,
//                                                   uint64_t value);
.global CompareAndExchange64
CompareAndExchange64:
  // returns 0 if success
  // returns 1 if timeout
  // args:
  //   rcx: dst addr
  //   rdx: expected
  //   r8: value
  // tmp:
  //   rax, r9
  mov r9, 0
CompareAndExchange64_try:
  inc r9
  cmp r9, 0x100
  jae CompareAndExchange64_timeout
	mov rax, rdx
  cmpxchg [rcx], r8
	jne CompareAndExchange64_sleep
  mov rax, 0
	ret
CompareAndExchange64_sleep:
  call Sleep
  jmp CompareAndExchange64_try
CompareAndExchange64_timeout:
  mov rax, 1
  ret

