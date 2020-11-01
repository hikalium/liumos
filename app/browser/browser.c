#include "../liumlib/liumlib.h"
#include "rendering.h"

typedef struct ParsedUrl {
  char *scheme;
  char *host;
  uint16_t port;
  char *path;
} ParsedUrl;

char *ip;
char *url;
char *html;
ParsedUrl *parsed_url;

void RequestLine(char* request) {
  strcpy(request, "GET ");
  strcat(request, parsed_url->path);
  strcat(request, " HTTP/1.1\n");
}

void Headers(char* request) {
  strcat(request, "Host: ");
  strcat(request, parsed_url->host);
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
  address.sin_port = htons(parsed_url->port);

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

ParsedUrl *ParseUrl() {
  ParsedUrl *parsed_url = (ParsedUrl *) malloc(sizeof(ParsedUrl));

  // Only support "http" scheme.
  if (strncmp("http://", url, 7) != 0) {
    Println("Error: Only support 'http' scheme.");
    exit(EXIT_FAILURE);
  }
  parsed_url->scheme = "http";

  // Parse `host`.
  int host_separator = 7;
  while (url[host_separator] && url[host_separator] != '/') {
    host_separator++;
  }
  int host_length = host_separator - 7;
  char *host = (char *) malloc(host_length + 1);
  memcpy(host, &url[7], host_length);
  host[host_length] = '\0';
  parsed_url->host = host;

  // Parse `ip` and `host` from `host`.
  for (int i=0; i<host_length; i++) {
    if (host[i] != ':')
      continue;

    char *tmp_ip = (char *) malloc(i+1);
    memcpy(tmp_ip, host, i);
    tmp_ip[i] = '\0';
    ip = tmp_ip;

    parsed_url->port = StrToNum16(&host[i+1], NULL);
  }

  char *path = (char *) malloc(strlen(url) - host_separator + 1);
  int url_idx = host_separator;
  int path_idx = 0;
  while (url[url_idx]) {
    path[path_idx] = url[url_idx];
    path_idx++;
    url_idx++;
  }
  path[path_idx] = '\0';
  parsed_url->path = path;

  return parsed_url;
}

// Return 1 when parse succeeded, return 2 when debug mode, otherwise return 0.
int ParseArgs(int argc, char** argv) {
  // Set default values.
  url = "http://localhost:8888/index.html";

  bool is_debug = 0;

  while (argc > 0) {
    if (strcmp("--url", argv[0]) == 0 || strcmp("-u", argv[0]) == 0) {
      url = argv[1];
      argc -= 2;
      argv += 2;
      continue;
    }

    if (strcmp("--rawtext", argv[0]) == 0) {
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
    Println("       -u, --url      URL. Default: http://localhost:8888/index.html");
    Println("           --rawtext  Raw HTML text for debug.");
    exit(EXIT_FAILURE);
  }

  // For debug.
  if (parse_result == 2) {
    html = (char *) malloc(SIZE_RESPONSE);
    html = argv[2];
    Render(html);
    exit(0);
  }

  parsed_url = ParseUrl();

  char *request = (char *) malloc(SIZE_REQUEST);
  char *response = (char *) malloc(SIZE_RESPONSE);
  html = (char *) malloc(SIZE_RESPONSE);
  BuildRequest(request);

  GetResponse(request, response);
  GetResponseBody(response, html);

  Render(html);

  exit(0);
}
