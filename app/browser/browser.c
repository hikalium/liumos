#include "../liumlib/liumlib.h"
#include "rendering.h"

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

void build_request(char *request) {
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
}

void get_response(char *request, char *response) {
  int socket_fd = 0;
  struct sockaddr_in address;

  if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
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
    write(1, "error: fail to connect socket\n", 30);
    close(socket_fd);
    exit(1);
    return;
  }
  sendto(socket_fd, request, strlen(request), 0, (struct sockaddr *) &address, sizeof(address));

  read(socket_fd, response, SIZE_RESPONSE);
  close(socket_fd);
}

struct ResponseHeader {
  char *key;
  char *value;
} response_headers[100];

void get_response_headers(char *response) {

}

void get_response_body(char *response, char *html) {
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
  char *html = (char *) malloc(SIZE_RESPONSE);

  if (argc == 1) {
    char *request = (char *) malloc(SIZE_REQUEST);
    char *response = (char *) malloc(SIZE_RESPONSE);
    build_request(request);
    get_response(request, response);
    get_response_body(response, html);
  } else if (argc == 3) {
    if (strcmp("-rawtext", argv[1]) == 0) {
      html = argv[2];
    } else {
      char *url = argv[1];
      host = strtok(url, "/");
      path = strtok(NULL, "/");
      ip = argv[2];

      char *request = (char *) malloc(SIZE_REQUEST);
      char *response = (char *) malloc(SIZE_RESPONSE);
      build_request(request);
      get_response(request, response);
      get_response_body(response, html);
    }
  } else {
    println("Usage: browser.bin");
    println("       browser.bin HOSTNAME IP");
    println("       browser.bin -rawtext RAW_TEXT");
    println("       * The default value of HOSTNAME and IP is localhost:8888");
    exit(1);
    return 1;
  }

  //tokenize(html);
  //print_tokens();
  render(html);

  exit(0);
  return 0;
}
