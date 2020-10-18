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
  AFTER_HEAD,
  IN_BODY,
  AFTER_BODY,
  AFTER_AFTER_BODY,
} Mode;

typedef enum ElementType {
  // https://html.spec.whatwg.org/multipage/dom.html#document
  DOCUMENT,
  // https://dom.spec.whatwg.org/#interface-text
  TEXT,
  HTML,
  HEAD,
  BODY, // HTMLBodyElement
  HEADING, // HTMLHeadingElement
  UL,
  LI,
} ElementType;

typedef struct Node {
  ElementType element_type;
  char *local_name;
  Attribute *attributes;
  char *data;
  struct Node *parent;
  struct Node *child;
  struct Node *previous;
  struct Node *next;
} Node;

Node *create_document();
Node *create_element(ElementType element_type, char *local_name);
Node *create_element_from_token(ElementType element_type, Token *token);
void construct_tree();
void print_node(Node *node); // for debug.
void print_nodes(); // for debug.

Node *root_node;
Node *current_node;
// https://html.spec.whatwg.org/multipage/parsing.html#the-stack-of-open-elements
Node *stack_of_open_elements;

#endif // APP_BROWSER_PARSE_H
