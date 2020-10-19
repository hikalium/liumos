#include "../liumlib/liumlib.h"
#include "rendering.h"
#include "lib.h"

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
  char *html;

  // For debug.
  if (argc == 3 && strcmp(argv[1], "-rawtext") == 0) {
    html = (char *) malloc(SIZE_RESPONSE);
    html = argv[2];
    render(html);
    exit(0);
    return 0;
  }

  if (argc != 1) {
    println("Usage: browser.bin");
    println("       browser.bin -rawtext RAW_HTML_TEXT");
    exit(1);
    return 1;
  }

  while (1) {
    host = NULL;
    path = NULL;
    ip = NULL;

    char *url = (char *) malloc (2048);
    write(1, "Input URL: ", 11);
    read(1, url, 2048);

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
    build_request(request);

    get_response(request, response);
    get_response_body(response, html);

    render(html);
    println("\n");
  }

  exit(0);
  return 0;
}
