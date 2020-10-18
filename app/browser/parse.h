// This is a part of "13.2.6 Tree construction" in the HTML spec.
// https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

#ifndef APP_BROWSER_PARSE_H
#define APP_BROWSER_PARSE_H

#include "tokenize.h"

// https://html.spec.whatwg.org/multipage/parsing.html#the-insertion-mode
typedef enum Mode {
  INITIAL,
  BEFORE_HTML,
  BEFORE_HEAD,
  IN_HEAD,
  IN_HEAD_NOSCRIPT,
  AFTER_HEAD,
  IN_BODY,
  TEXT,
  AFTER_BODY,
  AFTER_AFTER_BODY,
} Mode;

typedef struct Node {
  char *local_name;
  Dict attributes[10];
  char *data;
  struct Node *previous;
  struct Node *next;
} Node;

Node create_element(Token token);
void construct_tree();

Node *nodes;

#endif // APP_BROWSER_PARSE_H
