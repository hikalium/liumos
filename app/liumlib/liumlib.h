#ifndef LIB_H
#define LIB_H

#include <stdint.h>

#define NULL 0
#define true 1
#define false 0

#define EXIT_FAILURE 1

#define O_RDWR 2
#define O_CREAT (1 << 6)
#define AF_INET 2      // Internet IP protocol
#define SOCK_STREAM 1  // for TCP
#define SOCK_DGRAM 2   // for UDP
#define SOCK_RAW 3
#define SO_RCVTIMEO 20
#define SO_SNDTIMEO 21
#define SOL_SOCKET 1
#define PROT_ICMP 1
#define IPPROTO_UDP 17
#define PROT_WRITE 0x2
#define MAP_SHARED 0x01
#define MAP_FAILED ((void*)-1)
#define MS_SYNC 4

#define INADDR_ANY ((unsigned long int)0x00000000)

#define __bswap_16(x) ((uint16_t)((((x) >> 8) & 0xff) | (((x)&0xff) << 8)))

#define __bswap_32(x)                                   \
  ((((x)&0xff000000) >> 24) | (((x)&0x00ff0000) >> 8) | \
   (((x)&0x0000ff00) << 8) | (((x)&0x000000ff) << 24))

#define MALLOC_MAX_SIZE 1000000

#define SIZE_REQUEST 1000
#define SIZE_RESPONSE 2000

#undef assert
#define assert(expr) \
  ((void)((expr) || (__assert(#expr, __FILE__, __LINE__), 0)))

typedef long ssize_t;
typedef uint32_t in_addr_t;
typedef unsigned long size_t;
typedef _Bool bool;
typedef uint64_t off_t;

typedef uint32_t socklen_t;

// c.f.
// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L85
struct in_addr {
  uint32_t s_addr;
};

// c.f.
// https://elixir.bootlin.com/linux/v5.4.66/source/include/uapi/linux/time.h#L16
struct timeval {
  long tv_sec;
  long tv_usec;
};

struct timespec {
  long tv_sec;
  long tv_nsec;
};

// c.f.
// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L232
struct sockaddr_in {
  uint16_t sin_family;
  uint16_t sin_port;
  struct in_addr sin_addr;
  uint8_t padding[8];
  // c.f.
  // https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L231
};

// c.f.
// https://elixir.bootlin.com/linux/v5.4.66/source/include/linux/socket.h#L31
struct sockaddr {
  unsigned short sa_family; /* address family, AF_xxx   */
  char sa_data[14];         /* 14 bytes of protocol address */
};

// System call functions.
int ftruncate(int fd, off_t length);
int open(const char* pathname, int flags, int mode);
ssize_t read(int fd, void* buf, size_t count);
ssize_t write(int fd, const void* buf, size_t count);
int close(int fd);
int socket(int domain, int type, int protocol);
int connect(int sockfd, struct sockaddr* addr, socklen_t addrlen);
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);
ssize_t sendto(int sockfd,
               void* buf,
               size_t len,
               int flags,
               const struct sockaddr* dest_addr,
               socklen_t addrlen);
ssize_t recvfrom(int sockfd,
                 void* buf,
                 size_t len,
                 int flags,
                 struct sockaddr* src_addr,
                 socklen_t* addrlen);
int bind(int sockfd, struct sockaddr* addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int setsockopt(int sockfd,
               int level,
               int optname,
               const void* optval,
               socklen_t optlen);
void exit(int) __attribute__((noreturn));
void* mmap(void* addr,
           size_t length,
           int prot,
           int flags,
           int fd,
           off_t offset);
int msync(void* addr, size_t length, int flags);
int nanosleep(const struct timespec *, struct timespec *);

// Standard library functions.
void bzero(void* s, size_t n);
size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, unsigned long n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, unsigned long n);
char* strtok(char* str, const char* delim);
void* malloc(unsigned long n);
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
// convert values between host and network byte order.
uint16_t htons(uint16_t hostshort);
uint32_t htonl(uint32_t hostlong);
// Converts the Internet host address cp from IPv4 numbers-and-dots notation
// into binary data in network byte order.
uint32_t inet_addr(const char* cp);

// Compilier builtin standard library functions
void* memchr(const void*, int, unsigned long);
int memcmp(const void*, const void*, unsigned long);
char* strncpy(char*, const char*, unsigned long);
char* strrchr(const char*, int);
long strtol(const char*, char**, int);
int sprintf(char*, const char*, ...);

// architecture specific, for x86_64
typedef long long jmp_buf[8];

// liumlib original functions
void __attribute__((noreturn)) NotImplemented(const char* s);
void Print(const char* s);
void Println(const char* s);
void PrintNum(int v);
char NumToHexChar(char v);
uint8_t StrToByte(const char* s, const char** next);
uint16_t StrToNum16(const char* s, const char** next);
void __assert(const char* expr_str, const char* file, int line);
in_addr_t MakeIPv4Addr(uint8_t a, uint8_t b, uint8_t c, uint8_t d);
in_addr_t MakeIPv4AddrFromString(const char* s);
void PrintHex8ZeroFilled(uint8_t v);
void PrintIPv4Addr(in_addr_t addr);
void panic(const char* s);

static inline int puts(const char *s) {
  Println(s);
  return strlen(s);
}


#endif /* GRANDPARENT_H */
