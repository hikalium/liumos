// This is a part of "13.2.5 Tokenization" in the HTML spec.
// https://html.spec.whatwg.org/multipage/parsing.html#tokenization

#include "tokenize.h"

#include "rendering.h"
#include "lib.h"

Token *first_token = NULL;
Token *current_token = NULL;

void append_token(Token *token) {
  if (first_token == NULL) {
    first_token = token;
  } else {
    current_token->next = token;
  }
  current_token = token;
}

Token *create_token(TokenType type, char *tag, bool self_closing, char *data) {
  Token *token = (Token *) malloc(sizeof(Token));
  token->type = type;
  if (tag)
    token->tag_name = tag;
  token->self_closing = self_closing;
  token->attributes = NULL;
  if (data)
    token->data = data;
  token->next = NULL;
  return token;
}

char *append_doctype(char *html) {
  while (*html) {
    if (*html == '>') {
      append_token(create_token(DOCTYPE, NULL, 0, NULL));
      return html;
    }
    html++;
  }
  return html;
}

char *append_end_tag(char *html) {
  char tmp_tag[100];
  int i = 0;
  while (*html) {
    if (*html == '>') {
      char *tag = (char *) malloc(i+1);
      tmp_tag[i] = '\0';
      strcpy(tag, tmp_tag);
      append_token(create_token(END_TAG, tag, 0, NULL));
      return html;
    }
    tmp_tag[i] = *html;
    i++;
    html++;
  }
  return html;
}

char *append_start_tag(char *html) {
  char tmp_tag[100];
  int i = 0;
  bool self_closing = 0;
  while (*html) {
    if (*html == '/') {
      self_closing = 1;
      html++;
      continue;
    }
    if (*html == '>') {
      char *tag = (char *) malloc(i+1);
      tmp_tag[i] = '\0';
      strcpy(tag, tmp_tag);
      append_token(create_token(START_TAG, tag, self_closing, NULL));
      return html;
    }
    tmp_tag[i] = *html;
    i++;
    html++;
  }
  return html;
}

void append_char(char *tmp_char, int i) {
  char *data = (char *) malloc(i+1);
  tmp_char[i] = '\0';
  strcpy(data, tmp_char);
  append_token(create_token(CHAR, NULL, 0, data));
}

// https://html.spec.whatwg.org/multipage/parsing.html#tokenization
// "The output of the tokenization step is a series of zero or more of the following tokens:
// DOCTYPE, start tag, end tag, comment, character, end-of-file"
void tokenize(char *html) {
  char tmp_char[10000];
  int i = 0;
  while (*html) {
    switch (*html) {
      case '<':
        html++; // consume '<'.

        if (i > 0) {
          // If character buffer is not empty, append a char token.
          append_char(tmp_char, i);
          i = 0;
        }

        // tag open state.
        if (*html == '!') {
          html++; // consume '!'.
          html = append_doctype(html);
        } else if (*html == '/') {
          html++; // consume '/'.
          html = append_end_tag(html);
        } else {
          html = append_start_tag(html);
        }
        break;
      default:
        tmp_char[i] = *html;
        i++;
    }
    html++;
  }
  if (i > 0) {
    // If character buffer is not empty, append a char token.
    append_char(tmp_char, i);
    i = 0;
  }
  append_token(create_token(EOF, NULL, 0, NULL));
}

// for debug.
void print_tokens() {
  for (Token *token = first_token; token; token = token->next) {
    println("token:");
    switch (token->type) {
      case START_TAG:
      case END_TAG:
        println(token->tag_name);
        break;
      case CHAR:
        println(token->data);
        break;
      case EOF:
        println("EOF: End of file");
        break;
      default:
        break;
    }
  }
}
