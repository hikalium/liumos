// HTTP server.

#include "../liumlib/liumlib.h"

static void print_num(int v) {
  char s[16];
  int i;
  if (v < 0) {
    write(1, "-", 1);
    v = -v;
  }
  for (i = sizeof(s) - 1; i > 0; i--) {
    s[i] = v % 10 + '0';
    v /= 10;
    if (!v)
      break;
  }
  write(1, &s[i], sizeof(s) - i);
  write(1, "\n", 1);
}

static void print(char* text) {
  write(1, text, strlen(text));
}

static void println(char* text) {
  char output[100000];
  int i = 0;
  while (text[i] != '\0') {
    output[i] = text[i];
    i++;
  }
  write(1, output, i + 1);
  write(1, "\n", 1);
}

static uint16_t str_to_num16(const char* s, const char** next) {
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

void status_line(char *response, int status) {
  switch (status) {
    case 200:
      strcpy(response, "HTTP/1.1 200 OK\n");
      break;
    case 404:
      strcpy(response, "HTTP/1.1 404 Not Found\n");
      break;
    case 500:
      strcpy(response, "HTTP/1.1 500 Internal Server Error\n");
      break;
    case 501:
      strcpy(response, "HTTP/1.1 501 Not Implemented\n");
      break;
    default:
      strcpy(response, "HTTP/1.1 404 Not Found\n");
  }
}

void headers(char *response) {
  strcat(response, "Content-Type: text/html; charset=UTF-8\n");
}

void crlf(char *response) {
  strcat(response, "\n");
}

void body(char *response, char *message) {
  strcat(response, message);
}

void build_response(char *response, int status, char *message) {
  // https://tools.ietf.org/html/rfc7230#section-3
  // HTTP-message = start-line
  //                *( header-field CRLF )
  //                CRLF
  //                [ message-body ]
  status_line(response, status);
  headers(response);
  crlf(response);
  body(response, message);
}

void route(char *response, char *path) {
  if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
    char *body =
        "<html>"
        "  <body>"
        "    <h1>Hello World</h1>"
        "    <div>"
        "       <p>サンプルパラグラフです。</p>"
        "       <ul>"
        "           <li>リスト1</li>"
        "           <li>リスト2</li>"
        "       </ul>"
        "   </div>"
        " </body>"
        "</html>";
    build_response(response, 200, body);
    return;
  }
  if (strcmp(path, "/example.html") == 0) {
    char *body =
        "<html>"
        "  <body>"
        "    <h1>Example Page</h1>"
        "    <div>"
        "       <p>サンプルパラグラフです。</p>"
        "       <ul>"
        "           <li>リスト1</li>"
        "           <li>リスト2</li>"
        "       </ul>"
        "   </div>"
        " </body>"
        "</html>";
    build_response(response, 200, body);
    build_response(response, 200, "<html><body><h1>example page</h1><ul><li>abc</li><li>def</li></ul></body></html>");
    return;
  }
  char *body =
      "<html>"
      "  <body>"
      "    <p>Page is not found.</p>"
      " </body>"
      "</html>";
  build_response(response, 404, body);
}

void start(uint16_t port) {
  int socket_fd;
  struct sockaddr_in address;
  int addrlen = sizeof(address);

  if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
    write(1, "error: fail to create socket\n", 29);
    close(socket_fd);
    exit(1);
    return;
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(socket_fd, (struct sockaddr *) &address, addrlen) != 0) {
    println("Error: Failed to bind socket");
    close(socket_fd);
    exit(EXIT_FAILURE);
    return;
  }
  print("Listening port: ");
  print_num(port);
  print("\n");

  while (1) {
    println("Log: Waiting for a request...\n");

    char request[SIZE_REQUEST];
    unsigned int len = sizeof(address);
    if (recvfrom(socket_fd, request, SIZE_REQUEST, MSG_WAITALL,
                 (struct sockaddr*) &address, &len) < 0) {
      println("Error: Failed to receive a request.");
      close(socket_fd);
      exit(EXIT_FAILURE);
      return;
    }
    println(request);

    char *method = strtok(request, " ");
    char *path = strtok(NULL, " ");
    char *protocol = strtok(NULL, " ");

    char* response = (char *) malloc(SIZE_RESPONSE);

    if (strcmp(method, "GET") == 0) {
      route(response, path);
    } else {
      build_response(response, 501, "Methods not GET are not supported.");
    }

    if (sendto(socket_fd, response, strlen(response), MSG_CONFIRM,
               (struct sockaddr *) &address, addrlen) == -1) {
      println("Error: Failed to send a response.");
      close(socket_fd);
      exit(EXIT_FAILURE);
      return;
    }
  }

  close(socket_fd);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    println("Usage: httpserver.bin [ PORT ]");
    exit(EXIT_FAILURE);
    return EXIT_FAILURE;
  }

  uint16_t port = str_to_num16(argv[1], NULL);
  start(port);
  exit(0);
  return 0;
}
