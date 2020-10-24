// This is a part of "13.2.5 Tokenization" in the HTML spec.
// https://html.spec.whatwg.org/multipage/parsing.html#tokenization

#include "tokenize.h"

#include "rendering.h"
#include "lib.h"

Token *first_token = NULL;
Token *current_token = NULL;

Token *create_token(TokenType type) {
  Token *token = (Token *) malloc(sizeof(Token));
  token->type = type;
  if (type == START_TAG || type == END_TAG) {
    char *tag_name = (char *) malloc(20);
    token->tag_name = tag_name;
  }
  token->self_closing = 0;
  token->attributes = NULL;
  token->data = NULL;
  token->next = NULL;
  return token;
}

Token *create_char_token(char s) {
  Token *token = (Token *) malloc(sizeof(Token));
  char *data = (char *) malloc(2);
  data[0] = s;
  data[1] = '\0';

  token->type = CHAR;
  token->tag_name = NULL;
  token->self_closing = 0;
  token->attributes = NULL;
  token->data = data;
  token->next = NULL;
  return token;
}

void append_token(Token *token) {
  if (first_token == NULL) {
    first_token = token;
  } else {
    current_token->next = token;
  }
  current_token = token;
}

// https://html.spec.whatwg.org/multipage/parsing.html#tokenization
// "The output of the tokenization step is a series of zero or more of the following tokens:
// DOCTYPE, start tag, end tag, comment, character, end-of-file"
void tokenize(char *html) {
  first_token = NULL;
  current_token = NULL;

  State state = DATA;

  while (*html) {
    // Preprocess.
    if (*html == 0x000D) {
      // https://html.spec.whatwg.org/multipage/parsing.html#preprocessing-the-input-stream
      // "Before the tokenization stage, the input stream must be preprocessed by
      // normalizing newlines. Thus, newlines in HTML DOMs are represented by U+000A
      // LF characters, and there are never any U+000D CR characters in the input to
      // the tokenization stage."
      *html = 0x000A;
    }

    switch (state) {
      case DATA:
        // https://html.spec.whatwg.org/multipage/parsing.html#data-state
        if (*html == '<') {
          // U+003C LESS-THAN SIGN (<)
          // Switch to the tag open state.
          state = TAG_OPEN;
          html++;
          break;
        }
        // Anything else
        append_token(create_char_token(*html));
        html++;
        break;
      case TAG_OPEN:
        // https://html.spec.whatwg.org/multipage/parsing.html#tag-open-state
        if (*html == '/') {
          // U+002F SOLIDUS (/)
          // Switch to the end tag open state.
          state = END_TAG_OPEN;
          html++;
          break;
        }
        if (('a' <= *html && *html <= 'z') || ('A' <= *html && *html <= 'Z')) {
          // ASCII alpha
          // Create a new start tag token, set its tag name to the empty string.
          // Reconsume in the tag name state.
          append_token(create_token(START_TAG));
          state = TAG_NAME;
          break;
        }
        // Anything else
        // This is an invalid-first-character-of-tag-name parse error.
        // Emit a U+003C LESS-THAN SIGN character token. Reconsume in the data state.
        append_token(create_char_token('<'));
        state = DATA;
        break;
      case END_TAG:
        // https://html.spec.whatwg.org/multipage/parsing.html#end-tag-open-state
        if (('a' <= *html && *html <= 'z') || ('A' <= *html && *html <= 'Z')) {
          // ASCII alpha
          // Create a new end tag token, set its tag name to the empty string.
          // Reconsume in the tag name state.
          append_token(create_token(END_TAG));
          state = TAG_NAME;
          break;
        }
        html++;
        break;
      case TAG_NAME:
        // https://html.spec.whatwg.org/multipage/parsing.html#tag-name-state
        if (*html == '/') {
          // U+002F SOLIDUS (/)
          // Switch to the self-closing start tag state.
          state = SELF_CLOSING_START_TAG;
          html++;
          break;
        }
        if (*html == '>') {
          // U+003E GREATER-THAN SIGN (>)
          // Switch to the data state. Emit the current tag token.
          state = DATA;
          html++;
          break;
        }
        if ('A' <= *html && *html <= 'Z') {
          // ASCII upper alpha
          // Append the lowercase version of the current input character
          // (add 0x0020 to the character's code point) to the current tag token's tag name.
          *html = *html + ('A' - 'a');
          strncat(current_token->tag_name, html, 1);
          html++;
          break;
        }
        strncat(current_token->tag_name, html, 1);
        html++;
        break;
      case SELF_CLOSING_START_TAG:
        // https://html.spec.whatwg.org/multipage/parsing.html#self-closing-start-tag-state
        if (*html == '>') {
          // U+003E GREATER-THAN SIGN (>)
          // Set the self-closing flag of the current tag token.
          // Switch to the data state. Emit the current tag token.
          current_token->self_closing = 1;
          state = DATA;
          html++;
          break;
        }
        html++;
        break;
    }
  }
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
