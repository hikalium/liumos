#include "../liumlib/liumlib.h"
#include "rendering.h"

char* host;
char* path;
char* ip;
uint16_t port;

char *url;
char *html;

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

void ParseResponse(char *response, char *body) {

}

void ParseUrl() {
  host = "localhost:8888";
  path = "/index.html";
  ip = "127.0.0.1";
  port = 8888;
}

// Return 1 when parse succeeded, return 2 when debug mode, otherwise return 0.
int ParseArgs(int argc, char** argv) {
  // Set default values.
  url = "http://localhost:8888/index.html";

  bool is_debug = 0;

  while (argc > 0) {
    if (strcmp("-url", argv[0]) == 0) {
      url = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("-rawtext", argv[0]) == 0) {
      html = argv[1];
      is_debug = 1;
      argc -= 2;
      argv += 2;
      continue;
    }

    return 0;
  }

  if (is_debug)
    return 2;
  return 1;
}

int main(int argc, char *argv[]) {
  int parse_result = ParseArgs(argc-1, argv+1);
  if (parse_result == 0) {
    Println("Usage: browser.bin [ OPTIONS ]");
    Println("       -url      URL. Default: http://localhost:8888/index.html");
    Println("       -rawtext  Raw HTML text for debug.");
    exit(EXIT_FAILURE);
  }

  if (parse_result == 2) {
    html = (char *) malloc(SIZE_RESPONSE);
    html = argv[2];
    render(html);
    exit(0);
  }

  ParseUrl();

  char *request = (char *) malloc(SIZE_REQUEST);
  char *response = (char *) malloc(SIZE_RESPONSE);
  html = (char *) malloc(SIZE_RESPONSE);
  BuildRequest(request);

  GetResponse(request, response);
  GetResponseBody(response, html);

  render(html);

  exit(0);
}
