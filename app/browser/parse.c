// This is a part of "13.2.6 Tree construction" in the HTML spec.
// https://html.spec.whatwg.org/multipage/parsing.html#tree-construction

#include "parse.h"

// https://html.spec.whatwg.org/multipage/parsing.html#create-an-element-for-the-token
Node create_element(Token token) {
  Node node = {
      // 2. Let local name be the tag name of the token.
      .local_name = token.tag_name,
      //.attributes = token.attributes,
      .data = token.data,
      .previous = NULL,
      .next = NULL};
  return node;
}

void construct_tree() {
  Mode mode = INITIAL;

  for (int i=0; i<t_index; i++) {
    Token token = tokens[i];

    switch (mode) {
      case INITIAL:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-initial-insertion-mode
        mode = BEFORE_HTML;
      case BEFORE_HTML:
        // https://html.spec.whatwg.org/multipage/parsing.html#the-before-html-insertion-mode
        switch (token.type) {
          case DOCTYPE:
            // Parse error. Ignore the token.
            break;
          case START_TAG:
            if (strcmp("html", token.tag_name) == 0) {

            }
            break;
          default:
            break;
        }
      case BEFORE_HEAD:
      case IN_HEAD:
      case IN_HEAD_NOSCRIPT:
      case AFTER_HEAD:
      case IN_BODY:
      case TEXT:
      case AFTER_BODY:
      case AFTER_AFTER_BODY:
        break;
    }
  }
}
