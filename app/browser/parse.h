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

typedef enum NodeType {
  // https://dom.spec.whatwg.org/#interface-document
  DOCUMENT,
  // https://dom.spec.whatwg.org/#interface-element
  ELEMENT,
  // https://dom.spec.whatwg.org/#interface-text
  TEXT,
} NodeType;

typedef enum ElementType {
  HTML,
  HEAD,
  BODY, // HTMLBodyElement
  HEADING, // HTMLHeadingElement
  DIV, // HTMLDivElement
  PARAGRAPH, // HTMLParagraphElement
  UL, // HTMLUListElement
  LI, // HTMLLIElement
  NONE, // Fake element type
} ElementType;

typedef struct Node {
  NodeType node_type;
  ElementType element_type;
  char *tag_name;
  char *data;
  struct Node *parent;
  struct Node *first_child;
  struct Node *last_child;
  struct Node *previous_sibling;
  struct Node *next_sibling;
} Node;

void PushStack(Node *node);
Node *PopStack();
Node *CurrentNode();
void InsertElement(Node *element);
void InsertCharFromToken(Token *token);
Node *CreateDocument();
Node *CreateElement(ElementType element_type, char *tag_name);
Node *CreateElementFromToken(ElementType element_type, Token *token);
Node *CreateText(Token *token);
void ConstructTree();
void PrintNode(); // for debug.
void PrintNodes(); // for debug.

extern Node *root_node;
// https://html.spec.whatwg.org/multipage/parsing.html#the-stack-of-open-elements
extern Node *stack_of_open_elements[100];
extern int stack_index;

#endif // APP_BROWSER_PARSE_H
