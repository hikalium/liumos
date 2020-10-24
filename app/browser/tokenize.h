// This is a part of "13.2.5 Tokenization" in the HTML spec.
// https://html.spec.whatwg.org/multipage/parsing.html#tokenization

#ifndef APP_BROWSER_TOKENIZE_H
#define APP_BROWSER_TOKENIZE_H

#include "../liumlib/liumlib.h"

typedef enum State {
  DATA,
  TAG_OPEN,
  END_TAG_OPEN,
  TAG_NAME,
  SELF_CLOSING_START_TAG,
} State;

typedef enum TokenType {
  // "DOCTYPE tokens have a name, a public identifier, a system identifier, and a force-quirks flag."
  DOCTYPE,
  // "Start and end tag tokens have a tag name, a self-closing flag, and a list of attributes, each of which has a name and a value."
  START_TAG,
  END_TAG,
  // "Comment and character tokens have data."
  CHAR,
  EOF,
} TokenType;

typedef struct Attribute {
  char *key;
  char *value;
  struct Attribute *next;
} Attribute;

typedef struct Token {
  TokenType type;
  char *tag_name; // for start/end tag.
  bool self_closing; // for start/end tag.
  Attribute *attributes; // for start/end tag.
  char *data; // for character.
  struct Token *next;
} Token;

Token *create_token(TokenType type);
Token *create_char_token(char s);
void append_token(Token *token);
void tokenize(char *html);
void print_token(); // for debug.
void print_tokens(); // for debug.

extern Token *first_token;
extern Token *current_token;

#endif // APP_BROWSER_TOKENIZE_H
