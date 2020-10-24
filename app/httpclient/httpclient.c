// HTTP client.

#include "../liumlib/liumlib.h"

#define MSG_WAITALL 0x100

char* host = NULL;
char* path = NULL;
char* ip = NULL;
uint16_t port;

void request_line(char* request) {
  if (path) {
    strcpy(request, "GET /");
    strcat(request, path);
    strcat(request, " HTTP/1.1\n");
    return;
  }
  strcpy(request, "GET / HTTP/1.1\n");
}

void headers(char* request) {
  if (host) {
    strcat(request, "Host: ");
    strcat(request, host);
    strcat(request, "\n");
    return;
  }
  strcat(request, "Host: localhost:8888\n");
}

void crlf(char* request) {
  strcat(request, "\n");
}

static void Print(const char* s) {
  write(1, s, strlen(s));
}


void body(char* request) {}

void send_request(char* request) {
  int socket_fd = 0;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
  char response[SIZE_RESPONSE];

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    write(1, "error: fail to create socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  struct sockaddr_in dst_address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_port = htons(port);

  sendto(socket_fd, request, strlen(request), 0, (struct sockaddr*)&address,
         addrlen);
  Print("Request sent. waiting for a response...\n");

  unsigned int len = sizeof(address);
  int size = recvfrom(socket_fd, response, SIZE_RESPONSE, MSG_WAITALL,
                      (struct sockaddr*)&address, &len);
  response[size] = '\0';
  write(1, response, size);
  write(1, "\n", 1);

  close(socket_fd);
}

void println(char* text) {
  char output[100000];
  int i = 0;
  while (text[i] != '\0') {
    output[i] = text[i];
    i++;
  }
  write(1, output, i + 1);
  write(1, "\n", 1);
}

static uint16_t StrToNum16(const char* s, const char** next) {
  uint32_t v = 0;
  while ('0' <= *s && *s <= '9') {
    v = v * 10 + *s - '0';
    s++;
  }
  if (next) {
    *next = s;
  }
  return v;
}

int main(int argc, char** argv) {
  if (argc != 1 && argc != 3) {
    println("Usage: httpclient.bin [ IP PORT ]");
    println(
        "       IP and PORT are optional and default values are: IP=127.0.0.1 "
        "and PORT=8888");
    exit(1);
    return 1;
  }

  if (argc == 3) {
    ip = argv[1];
    port = StrToNum16(argv[2], NULL);
  } else {
    ip = "127.0.0.1";
    port = 8888;
  }

  char* request = (char*)malloc(SIZE_REQUEST);

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
