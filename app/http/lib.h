#ifndef LIB_H
#define LIB_H

#include "stdint.h"

#define AF_INET 2 // Internet IP protocol
#define SOCK_STREAM 1 // Stream (connection) socket

#define INADDR_ANY ((unsigned long int) 0x00000000)

#define PORT 8080

#define __bswap_16(x) \
  ((__uint16_t) ((((x) >> 8) & 0xff) | (((x) & 0xff) << 8)))

#define MALLOC_MAX_SIZE 100000

typedef long ssize_t;
typedef unsigned long size_t;
typedef _Bool bool;

typedef uint32_t socklen_t;

int malloc_size;
char malloc_array[MALLOC_MAX_SIZE];

// c.f.
// https://elixir.bootlin.com/linux/v4.15/source/include/uapi/linux/in.h#L85
struct in_addr {
  uint32_t s_addr;
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

// System call functions.
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, void *buf, size_t count);
int close(int fd);
int socket(int domain, int type, int protocol);
int connect(int sockfd, struct sockaddr_in *addr,
            socklen_t addrlen);
int accept(int sockfd, struct sockaddr_in *addr, socklen_t *addrlen);
ssize_t sendto(int sockfd,
               void *buf,
               size_t len,
               int flags,
               struct sockaddr_in *dest_addr,
               socklen_t addrlen);
int bind(int sockfd, struct sockaddr_in *addr,
         socklen_t addrlen);
int listen(int sockfd, int backlog);
int setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
//ssize_t send(int sockfd, void* buf, size_t len, int flags);
void exit(int);

// Standard library functions.
size_t my_strlen(char *s);
char *my_strcpy(char *dest, char *src);
// host to network short.
uint16_t htons(uint16_t hostshort);
void *my_malloc(int n);

#endif /* GRANDPARENT_H */
