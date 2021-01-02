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

.global write
write:
	mov rax, 1
	syscall
	ret

// int ftruncate(int fd, off_t length);
.global ftruncate
ftruncate:
	// arg[1]: rdi = rdi
	// arg[2]: rsi = rsi
	mov rax, 77
	syscall
	ret

// int open(const char* pathname, int flags, int mode);
.global open
open:
	// arg[1]: rdi = rdi
	// arg[2]: rsi = rsi
	// arg[3]: rdx = rdx
	mov rax, 2
	syscall
	ret

// void* mmap(void* addr,
//            size_t length,
//            int prot,
//            int flags,
//            int fd,
//            off_t offset);
.global mmap
mmap:
	mov rax, 9
	// arg[1]: rdi = rdi
	// arg[2]: rsi = rsi
	// arg[3]: rdx = rdx
	mov r10, rcx
	// arg[5]: r8 = r8
	// arg[6]: r9 = r9
	syscall
	ret

// int msync(void* addr, size_t length, int flags);
.global msync
msync:
	// arg[1]: rdi = rdi
	// arg[2]: rsi = rsi
	// arg[3]: rdx = rdx
	mov rax, 26
	syscall
	ret

// int socket(int domain, int type, int protocol);
.global socket
socket:
	// arg[1]: rdi = rdi
	// arg[2]: rsi = rsi
	// arg[3]: rdx = rdx
	mov rax, 41
	syscall
	ret

// ssize_t sendto(
//     int sockfd, const void *buf,
//     size_t len, int flags,
//     const struct sockaddr *dest_addr,
//     socklen_t addrlen);
.global sendto
sendto:
	mov rax, 44
	// arg[1]: rdi = rdi
	// arg[2]: rsi = rsi
	// arg[3]: rdx = rdx
	mov r10, rcx
	// arg[5]: r8 = r8
	// arg[6]: r9 = r9
	syscall
	ret

// ssize_t recvfrom(
//     int sockfd, void *buf,
//     size_t len, int flags,
//     struct sockaddr *src_addr,
//     socklen_t addrlen);
.global recvfrom
recvfrom:
	mov rax, 45
	// arg[1]: rdi = rdi
	// arg[2]: rsi = rsi
	// arg[3]: rdx = rdx
	mov r10, rcx
	// arg[5]: r8 = r8
	// arg[6]: r9 = r9
	syscall
	ret

// int close(int fd);
.global close
close:
	mov rax, 3
	// arg[1]: rdi = rdi
	syscall
	ret

.global exit
exit:
	mov rax, 60
	syscall

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
