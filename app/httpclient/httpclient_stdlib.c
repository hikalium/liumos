// HTTP client with standard library.

#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 8888
#define SIZE_REQUEST 1000
#define SIZE_RESPONSE 10000

char *host = NULL;
char *path = NULL;
char *ip = NULL;

void request_line(char *request) {
  if (path) {
    strcpy(request, "GET /");
    strcat(request, path);
    strcat(request, " HTTP/1.1\n");
    return;
  }
  strcpy(request, "GET / HTTP/1.1\n");
}

void headers(char *request) {
  if (host) {
    strcat(request, "Host: ");
    strcat(request, host);
    strcat(request, "\n");
    return;
  }
  strcat(request, "Host: localhost:8888\n");
}

void crlf(char *request) {
  strcat(request, "\n");
}

void body(char *request) {
}

void send_request(char *request) {
  int socket_fd = 0;
  struct sockaddr_in address;
  char response[SIZE_RESPONSE];

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket failed");
    write(1, "error: fail to create socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  address.sin_family = AF_INET;
  if (ip) {
    address.sin_addr.s_addr = inet_addr(ip);
    address.sin_port = htons(80);
  } else {
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
  }

  if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) == -1) {
    perror("connect failed");
    write(1, "error: fail to connect socket\n", 30);
    close(socket_fd);
    exit(1);
    return;
  }
  sendto(socket_fd, request, strlen(request), 0, (struct sockaddr *) &address, sizeof(address));

  int size = read(socket_fd, response, SIZE_RESPONSE);
  write(1, response, size);
  write(1, "\n", 1);

  close(socket_fd);
}

int main(int argc, char *argv[]) {
  if (argc != 1 && argc != 3) {
    write(1, "usage: client.bin [hostname] [ip]\n", 34);
    write(1, "       [hostname] and [ip] are optional and default value is localhost:8888\n", 76);
    exit(1);
    return 1;
  }

  if (argc == 3) {
    char *url = argv[1];
    host = strtok(url, "/");
    path = strtok(NULL, "/");
    ip = argv[2];
  }

  char* request = (char *) malloc(SIZE_REQUEST);

  // c.f.
  // https://tools.ietf.org/html/rfc7230#section-3
  // HTTP-message = start-line
  //                *( header-field CRLF )
  //                CRLF
  //                [ message-body ]
  request_line(request);
  headers(request);
  crlf(request);
  body(request);

  write(1, "----- request -----\n", 20);
  write(1, request, strlen(request));
  write(1, "----- response -----\n", 21);

  send_request(request);

  exit(0);
  return 0;
}
