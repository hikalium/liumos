// This is a part of "13.2.6 Tree construction" in the HTML spec.
// https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

#include "parse.h"

#include "../liumlib/liumlib.h"
#include "lib.h"

void append_child(Node *child) {
  child->parent = current_node;
  current_node->child = child;
  current_node = child;
}

void append_next() {

}

Node *create_document() {
  Node *node = (Node *) malloc(sizeof(Node));
  // Document is not an element, but for the sake of simplicity.
  node->element_type = DOCUMENT;
  node->local_name = NULL;
  node->attributes = NULL;
  node->data = NULL;
  node->child = NULL;
  node->previous = NULL;
  node->next = NULL;
  return node;
}

Node *create_element(ElementType element_type, char *local_name) {
  Node *node = (Node *) malloc(sizeof(Node));
  node->element_type = element_type;
  node->local_name = local_name;
  node->attributes = NULL;
  node->data = NULL;
  node->child = NULL;
  node->previous = NULL;
  node->next = NULL;
  return node;
}

// https://html.spec.whatwg.org/multipage/parsing.html#create-an-element-for-the-token
Node *create_element_from_token(ElementType element_type, Token *token) {
  Node *node = (Node *) malloc(sizeof(Node));
  node->element_type = element_type;
  // "2. Let local name be the tag name of the token."
  node->local_name = token->tag_name;
  node->attributes = token->attributes;
  node->data = token->data;
  node->child = NULL;
  node->previous = NULL;
  node->next = NULL;
  return node;
}

void construct_tree() {
  Mode mode = INITIAL;

  Node *document = create_document();
  root_node = document;
  current_node = document;

  for (int i=0; i<t_index; /* increment index inside the loop */) {
    Token token = tokens[i];

    switch (mode) {
      case INITIAL:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
        println("1111111111 initial mode");
        mode = BEFORE_HTML;
        break;
      case BEFORE_HTML:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
        println("2222222222 before html mode");
        if (token.type == DOCTYPE) {
          // Parse error. Ignore the token.
          i++;
          break;
        }
        if (token.type == START_TAG && strcmp(token.tag_name, "html") == 0) {
          Node *element = create_element_from_token(HTML, &token);
          append_child(element);
          mode = BEFORE_HEAD;
          i++; // Go to the next token.
          break;
        }
        if (token.type == END_TAG &&
            (strcmp(token.tag_name, "head") != 0 ||
             strcmp(token.tag_name, "body") != 0 ||
             strcmp(token.tag_name, "html") != 0 ||
             strcmp(token.tag_name, "br") != 0)) {
          // Any other end tag
          // Parse error. Ignore the token.
          i++;
          break;
        }
        // Anything else
        {
          Node *element = create_element(HTML, "html");
          append_child(element);
        }
        mode = BEFORE_HEAD;
        // Reprocess the token.
        break;
      case BEFORE_HEAD:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-before-head-insertion-mode
        println("3333333333 before head mode");
        if (token.type == DOCTYPE) {
          // A DOCTYPE token
          // Parse error. Ignore the token.
          break;
        }
        if (token.type == START_TAG && strcmp(token.tag_name, "head") == 0) {
          // A start tag whose tag name is "head"
          Node *element = create_element_from_token(HEAD, &token);
          append_child(element);
          mode = IN_HEAD;
          i++;
          break;
        }
        if (token.type == END_TAG &&
            (strcmp(token.tag_name, "head") != 0 ||
             strcmp(token.tag_name, "body") != 0 ||
             strcmp(token.tag_name, "html") != 0 ||
             strcmp(token.tag_name, "br") != 0)) {
          // Any other end tag
          // Parse error. Ignore the token.
          i++;
          break;
        }
        // Anything else
        {
          Node *element = create_element(HEAD, "head");
          append_child(element);
        }
        mode = IN_HEAD;
        break;
      case IN_HEAD:
        // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inhead
        println("44444444444 in head mode");
        if (token.type == DOCTYPE) {
          // A DOCTYPE token
          // Parse error. Ignore the token.
          break;
        }
        if (token.type == START_TAG && strcmp(token.tag_name, "html") == 0) {
          // A start tag whose tag name is "html"
          // Process the token using the rules for the "in body" insertion mode.
          mode = IN_BODY;
          i++;
          break;
        }
        if (token.type == START_TAG && strcmp(token.tag_name, "head") == 0) {
          // A start tag whose tag name is "head"
          // Parse error. Ignore the token.
          i++;
          break;
        }
        if (token.type == END_TAG && strcmp(token.tag_name, "head") == 0) {
          // An end tag whose tag name is "head"
          i++;
          mode = AFTER_HEAD;
          break;
        }
        if (token.type == END_TAG &&
            (strcmp(token.tag_name, "body") != 0 ||
            strcmp(token.tag_name, "html") != 0 ||
            strcmp(token.tag_name, "br") != 0)) {
          // Any other end tag
          // Parse error. Ignore the token.
          i++;
          break;
        }
        // Anything else
        mode = AFTER_HEAD;
        // Reprocess the token.
        break;
      case AFTER_HEAD:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-after-head-insertion-mode
        println("5555555555 after head mode");
        if (token.type == DOCTYPE) {
          // A DOCTYPE token
          // Parse error. Ignore the token.
          break;
        }
        if (token.type == START_TAG && strcmp(token.tag_name, "html") == 0) {
          // A start tag whose tag name is "html"
          // Process the token using the rules for the "in body" insertion mode.
          mode = IN_BODY;
          i++;
          break;
        }
        if (token.type == START_TAG && strcmp(token.tag_name, "body") == 0) {
          // A start tag whose tag name is "body"
          Node *element = create_element_from_token(BODY, &token);
          append_child(element);
          mode = IN_BODY;
          i++;
          break;
        }
        if (token.type == START_TAG && strcmp(token.tag_name, "head") == 0) {
          // A start tag whose tag name is "head"
          // Parse error. Ignore the token.
          i++;
          break;
        }
        if (token.type == END_TAG &&
            (strcmp(token.tag_name, "body") != 0 ||
            strcmp(token.tag_name, "html") != 0 ||
            strcmp(token.tag_name, "br") != 0)) {
          // Any other end tag
          // Parse error. Ignore the token.
          i++;
          break;
        }
        // Anything else
        {
          Node *element = create_element(BODY, "body");
          append_child(element);
        }
        mode = IN_BODY;
        // Reprocess the token.
        break;
      case IN_BODY:
        // https://html.spec.whatwg.org/multipage/parsing.html#parsing-main-inbody
        println("6666666666666 in body mode");
        if (token.type == START_TAG &&
            (strcmp(token.tag_name, "h1") == 0 ||
            strcmp(token.tag_name, "h2") == 0 ||
            strcmp(token.tag_name, "h3") == 0 ||
            strcmp(token.tag_name, "h4") == 0 ||
            strcmp(token.tag_name, "h5") == 0 ||
            strcmp(token.tag_name, "h6") == 0)) {
          // A start tag whose tag name is one of: "h1", "h2", "h3", "h4", "h5", "h6"
          Node *element = create_element_from_token(HEADING, &token);
          append_child(element);
          i++;
          break;
        }
        if (token.type == END_TAG) {
          // Any other end tag
          current_node = current_node->parent;
          i++;
          break;
        }
      case TEXT:
      case AFTER_BODY:
      case AFTER_AFTER_BODY:
        i++;
        break;
    }
  }
}

void print_node(Node *node) {
  switch (node->element_type) {
    case DOCUMENT:
      println("DOCUMENT");
      break;
    case HTML:
      println("HTML");
      break;
    case HEAD:
      println("HEAD");
      break;
    case BODY:
      println("BODY");
      break;
    default:
      println(node->local_name);
      break;
  }
}

void print_nodes() {
  Node *node = root_node;

  while (node) {
    print_node(node);

    Node *next = node->next;
    while (next) {
      print_node(next);
      next = next->next;
    }

    node = node->child;
  }
}
