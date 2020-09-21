// HTTP client with standard library.

#include <netinet/in.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <sys/socket.h> 
#include <unistd.h> 

#define PORT 8888

void send_request(char *request) {
  int socket_fd = 0;
  struct sockaddr_in address;
  char response[1024];

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket failed");
    write(1, "error: fail to create socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  address.sin_family = AF_INET;
  address.sin_port = htons(PORT);

  if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
    perror("connect failed");
    write(1, "error: fail to connect socket\n", 30);
    close(socket_fd);
    exit(1);
    return;
  }
  sendto(socket_fd, request, strlen(request), 0, (struct sockaddr *) &address, sizeof(address));

  int size = read(socket_fd, response, 1024);
  write(1, response, size);
  write(1, "\n", 1);

  close(socket_fd);
}

void start_line(char *request) {
  strcpy(request, "GET /index.html HTTP/1.1\n");
}

void headers(char *request) {
  strcat(request, "Host: example.com\n");
}

void crlf(char *request) {
  strcat(request, "\n");
}

void body(char *request) {
}

int main(int argc, char *argv[]) {
  char* request = (char *) malloc(1000);

  // https://tools.ietf.org/html/rfc7230#section-3
  // HTTP-message = start-line
  //                *( header-field CRLF )
  //                CRLF
  //                [ message-body ]
  start_line(request);
  headers(request);
  crlf(request);
  body(request);

  send_request(request);
  exit(0);
  return 0;
}
