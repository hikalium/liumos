.intel_syntax noprefix

// Syscall numbers:
// https://chromium.googlesource.com/chromiumos/docs/+/master/constants/syscalls.md#x86_64-64_bit

// System V AMD64 ABI:
//   args: RDI, RSI, RDX, RCX, R8, R9
//   callee-saved: RBX, RBP, R12, R13, R14, R15
//   caller-saved: otherwise

// Linux kernel x86-64 syscall:
//   idx: RAX
//   args: RDI, RSI, RDX, R10, R8, R9

// ssize_t read(int fd, void *buf, size_t count);
.global read
read:
    mov rax, 0x0
    syscall
    ret

// int connect(int sockfd, const struct sockaddr *addr,
//             socklen_t addrlen);
.global connect
connect:
	mov rax, 0x2a
    syscall
    ret

// int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
.global accept
accept:
	mov rax, 0x2b
    syscall
    ret

// int bind(int sockfd, const struct sockaddr *addr,
//          socklen_t addrlen);
.global bind
bind:
    mov rax, 0x31
    syscall
    ret

// int listen(int sockfd, int backlog);
.global listen
listen:
    mov rax, 0x32
    syscall
    ret

// int setsockopt(int sockfd, int level, int optname,
//                const void *optval, socklen_t optlen);
.global setsockopt
setsockopt:
    mov rax, 0x36
	mov r10, rcx
    syscall
    ret
