#ifndef TOKENIZE_H
#define TOKENIZE_H

#include "../liumlib/liumlib.h"

// https://html.spec.whatwg.org/multipage/parsing.html#tokenization
typedef enum TokenType {
  // "DOCTYPE tokens have a name, a public identifier, a system identifier, and a force-quirks flag."
  DOCTYPE,
  // "Start and end tag tokens have a tag name, a self-closing flag, and a list of attributes, each of which has a name and a value."
  START_TAG,
  END_TAG,
  // "Comment and character tokens have data."
  COMMENT,
  CHAR,
  EOF,
} TokenType;

typedef struct Dict {
  char *key;
  char *value;
} Dict;

typedef struct Token {
  TokenType type;
  char *tag_name; // for start/end tag.
  bool self_closing; // for start/end tag.
  Dict attributes[10]; // for start/end tag.
  char *data; // for comment and character.
} Token;

char *append_doctype(char *html);
char *append_end_tag(char *html);
char *append_start_tag(char *html);
void tokenize(char *html);
void print_tokens(); // for debug

#endif // TOKENIZE_H
