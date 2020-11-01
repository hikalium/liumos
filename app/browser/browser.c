#include "../liumlib/liumlib.h"
#include "rendering.h"
#include "lib.h"

char* host;
char* path;
char* ip;
uint16_t port;

void RequestLine(char* request) {
  strcpy(request, "GET ");
  strcat(request, path);
  strcat(request, " HTTP/1.1\n");
}

void Headers(char* request) {
  strcat(request, "Host: ");
  strcat(request, host);
  strcat(request, "\n");
}

void Crlf(char* request) {
  strcat(request, "\n");
}

void Body(char* request) {}

void BuildRequest(char *request) {
  // https://tools.ietf.org/html/rfc7230#section-3
  // HTTP-message = start-line
  //                *( header-field CRLF )
  //                CRLF
  //                [ message-body ]
  RequestLine(request);
  Headers(request);
  Crlf(request);
  Body(request);
}

void GetResponse(char* request, char *response) {
  int socket_fd = 0;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    Println("Error: Fail to create a socket");
    exit(EXIT_FAILURE);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip);
  address.sin_port = htons(port);

  if (sendto(socket_fd, request, strlen(request), 0,
             (struct sockaddr*)&address, addrlen) < 0) {
    Println("Error: Failed to send a request.");
    exit(EXIT_FAILURE);
  }

  unsigned int len = sizeof(address);
  if (recvfrom(socket_fd, response, SIZE_RESPONSE, 0,
               (struct sockaddr*)&address, &len) < 0) {
    Println("Error: Failed to receiver a response.");
    exit(EXIT_FAILURE);
  }

  close(socket_fd);
}

struct ResponseHeader {
  char *key;
  char *value;
} response_headers[100];

void GetResponseHeaders(char *response) {

}

void GetResponseBody(char *response, char *html) {
  while (*response) {
    // Assume a HTML tag comes.
    if (*response == '<') {
      strcpy(html, response);
      return;
    }
    response++;
  }
}

void parse_response(char *response, char *body) {

}

int main(int argc, char *argv[]) {
  char *html;

  // For debug.
  if (argc == 3 && strcmp(argv[1], "-rawtext") == 0) {
    html = (char *) malloc(SIZE_RESPONSE);
    html = argv[2];
    render(html);
    exit(0);
  }

  if (argc != 1) {
    println("Usage: browser.bin");
    println("       browser.bin -rawtext RAW_HTML_TEXT");
    exit(EXIT_FAILURE);
  }

  while (1) {
    host = NULL;
    path = NULL;
    ip = NULL;
    port = 8888;

    char *url = (char *) malloc (2048);
    write(1, "Input URL: ", 11);
    read(1, url, 2048);

    // parse url.
    if (strlen(url) > 1) {
      host = strtok(url, "/");
      path = strtok(NULL, "/");
      if (strlen(path) > 1 && path[strlen(path)-1] == '\n') {
        path[strlen(path)-1] = '\0';
      }
    }

    char *ip = (char *) malloc (2048);
    write(1, "Input IP: ", 10);
    read(1, ip, 2048);

    if (strlen(ip) <= 1) {
      ip = NULL;
    }

    char *request = (char *) malloc(SIZE_REQUEST);
    char *response = (char *) malloc(SIZE_RESPONSE);
    html = (char *) malloc(SIZE_RESPONSE);
    BuildRequest(request);

    GetResponse(request, response);
    GetResponseBody(response, html);

    render(html);
    println("\n");
  }

  exit(0);
  return 0;
}
