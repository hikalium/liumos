#ifndef RENDERING_H 
#define RENDERING_H 

#include "../liumlib/liumlib.h"

// This parsing model for HTML is a subset of the parsing model in the spec.
// https://html.spec.whatwg.org/multipage/parsing.html

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

typedef enum Tag {
  HTML,
  BODY,
  H1,
  H2,
  H3,
  H4,
  H5,
  H6,
  UL,
  LI,
  TEXT,
} Tag;

typedef struct Node {
  Tag tag;
  char *text;
} Node;

void generate();
void parse(char *html);
void render(char *html);

void println(char *text);

void append_doctype(char *html);
void append_end_tag(char *html);
void append_start_tag(char *html);
void tokenize(char *html);
void print_tokens(); // for debug

#endif // RENDERING_H
