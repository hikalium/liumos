// HTTP server with standard library.

#include <netinet/in.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <sys/socket.h> 
#include <unistd.h> 

#define PORT 8888

void start() {
  int socket_fd, accepted_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket failed");
    write(1, "error: fail to create socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  if (bind(socket_fd, (struct sockaddr *) &address, addrlen) == -1) {
    perror("bind failed");
    write(1, "error: fail to bind socket\n", 27);
    close(socket_fd);
    exit(1);
    return;
  }

  if (listen(socket_fd, 3) < 0) {
    perror("listen failed");
    write(1, "error: fail to listen socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  while (1) {
    write(1, "LOG: wait a message from client\n", 32);
    if ((accepted_socket = accept(socket_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) == -1) {
      write(1, "error: fail to accept socket\n", 29);
      close(socket_fd);
      close(accepted_socket);
      exit(1);
      return;
    }

    char request[1024];
    int size = read(accepted_socket, request, 1024);
    write(1, request, size);

    char* response = "HTTP/1.1 200 OK";
    sendto(accepted_socket, response, strlen(response), 0, (struct sockaddr *) &address, addrlen);

    close(accepted_socket);
  }
}

void route() {

}

int main(int argc, char *argv[]) {
  start();
  exit(0);
  return 0;
}
