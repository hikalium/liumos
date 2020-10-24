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

void append_char(char tmp_char) {
  char *data = (char *) malloc(2);
  data[0] = tmp_char;
  data[1] = '\0';
  append_token(create_token(CHAR, NULL, 0, data));
}

// https://html.spec.whatwg.org/multipage/parsing.html#tokenization
// "The output of the tokenization step is a series of zero or more of the following tokens:
// DOCTYPE, start tag, end tag, comment, character, end-of-file"
void tokenize(char *html) {
  first_token = NULL;
  current_token = NULL;

  while (*html) {
    switch (*html) {
      // Preprocess.
      if (*html == 0x000D) {
        // https://html.spec.whatwg.org/multipage/parsing.html#preprocessing-the-input-stream
        // "Before the tokenization stage, the input stream must be preprocessed by
        // normalizing newlines. Thus, newlines in HTML DOMs are represented by U+000A
        // LF characters, and there are never any U+000D CR characters in the input to
        // the tokenization stage."
        *html = 0x000A;
      }

      case '<':
        html++; // consume '<'.

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
        append_char(*html);
    }
    html++;
  }
  append_token(create_token(EOF, NULL, 0, NULL));
}

// for debug.
void print_token(Token *token) {
  switch (token->type) {
    case START_TAG:
      write(1, "start: ", 7);
      println(token->tag_name);
      break;
    case END_TAG:
      write(1, "end: ", 5);
      println(token->tag_name);
      break;
    case CHAR:
      write(1, "char: ", 6);
      println(token->data);
      break;
    case EOF:
      println("EOF: End of file");
      break;
    default:
      break;
  }
}

// for debug.
void print_tokens() {
  println("==============");
  for (Token *token = first_token; token; token = token->next) {
    print_token(token);
  }
  println("==============");
}
