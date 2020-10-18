// This is a part of "13.2.5 Tokenization" in the HTML spec.
// https://html.spec.whatwg.org/multipage/parsing.html#tokenization

#include "tokenize.h"

#include "rendering.h"
#include "lib.h"

Token tokens[100];
int t_index = 0;

char *append_doctype(char *html) {
  while (*html) {
    if (*html == '>') {
      Token token = { .type = DOCTYPE };
      tokens[t_index] = token;
      t_index++;
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
      Token token = { .type = END_TAG, .tag_name = tag };
      tokens[t_index] = token;
      t_index++;
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
      Token token = { .type = START_TAG, .tag_name = tag, .self_closing = self_closing };
      tokens[t_index] = token;
      t_index++;
      return html;
    }
    tmp_tag[i] = *html;
    i++;
    html++;
  }
  return html;
}

void append_char(char *tmp_char, int i) {
  char *c = (char *) malloc(i+1);
  tmp_char[i] = '\0';
  strcpy(c, tmp_char);
  Token token = { .type = CHAR, .data = c };
  tokens[t_index] = token;
  t_index++;
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
  Token token = { .type = EOF };
  tokens[t_index] = token;
  t_index++;
}

// for debug.
void print_tokens() {
  for (int i=0; i<t_index; i++) {
    println("token:");
    switch (tokens[i].type) {
      case START_TAG:
      case END_TAG:
        println(tokens[i].tag_name);
        break;
      case CHAR:
        println(tokens[i].data);
        break;
      case EOF:
        println("EOF: End of file");
        break;
      default:
        break;
    }
  }
}
