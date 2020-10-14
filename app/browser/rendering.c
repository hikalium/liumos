#include "rendering.h"

#include "../liumlib/liumlib.h"

int nodei;
struct Node nodes[100];

void println(char *text) {
  char output[100000];
  int i = 0;
  while (text[i] != '\0') {
    output[i] = text[i];
    i++;
  }
  write(1, output, i);
  write(1, "\n", 1);
}

void generate() {
  for (int i=0; i<nodei; i++) {
    switch (nodes[i].tag) {
      case H1:
        write(1, "# ", 2);
        break;
      case H2:
        write(1, "## ", 3);
        break;
      case H3:
        write(1, "### ", 4);
        break;
      case H4:
        write(1, "#### ", 5);
        break;
      case H5:
        write(1, "##### ", 6);
        break;
      case H6:
        write(1, "###### ", 7);
        break;
      case LI:
        write(1, "- ", 2);
        break;
      case TEXT:
        println(nodes[i].text);
        break;
      case HTML:
      case BODY:
      case UL:
      default:
        break;
    }
  }
}

Node create_node(char *tag_text, char *text) {
  Tag tag;
  if (strncmp(tag_text, "html", 4) == 0) {
    tag = HTML;
  } else if (strncmp(tag_text, "body", 4) == 0) {
    tag = BODY;
  } else if (strncmp(tag_text, "h1", 2) == 0) {
    tag = H1;
  } else if (strncmp(tag_text, "h2", 2) == 0) {
    tag = H2;
  } else if (strncmp(tag_text, "h3", 2) == 0) {
    tag = H3;
  } else if (strncmp(tag_text, "h4", 2) == 0) {
    tag = H4;
  } else if (strncmp(tag_text, "h5", 2) == 0) {
    tag = H5;
  } else if (strncmp(tag_text, "h6", 2) == 0) {
    tag = H6;
  } else if (strncmp(tag_text, "ul", 2) == 0) {
    tag = UL;
  } else if (strncmp(tag_text, "li", 2) == 0) {
    tag = LI;
  } else if (strncmp(tag_text, "text", 4) == 0) {
    tag = TEXT;
  }
  Node node = {tag, text};
  return node;
}

int parse_text(char *html, int i) {
  char *text = (char *) malloc(1000);
  int j = 0;
  while (html[i] != '<' && html[i] != '/' && html[i] != '\0') {
    text[j] = html[i];
    i++;
    j++;
  }
  text[j] = '\0';
  nodes[nodei] = create_node("text", text);
  return i;
}

int skip_closing_tag(char *html, int i) {
  while (html[i] != '>') {
    i++;
  }
  // skip '>' and return the next index.
  return ++i;
}

int parse_element(char *html, int i) {
  char tag[10];
  int j = 0;
  while (html[i] != '>') {
    tag[j] = html[i];
    i++;
    j++;
  }
  nodes[nodei] = create_node(tag, "");
  // skip '>' and return the next index.
  return ++i;
}

void parse(char *html) {
  int i = 0;
  while (html[i] != '\0') {
    if ((html[i] == '<' && html[i+1] == '/') || html[i] == '/') {
      i++;
      i = skip_closing_tag(html, i);
    } else if (html[i] == '<') {
      i++;
      i = parse_element(html, i);
      nodei++;
    } else {
      i = parse_text(html, i);
      nodei++;
    }
  }
}

Token tokens[100];
int t_index = 0;

void append_doctype(char *html) {
  while (*html) {
    if (*html == '>') {
      Token token = { .type = DOCTYPE };
      tokens[t_index] = token;
      t_index++;
      html++;
      return;
    }
    html++;
  }
}

void append_end_tag(char *html) {
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
      html++;
      return;
    }
    tmp_tag[i] = *html;
    i++;
    html++;
  }
}

void append_start_tag(char *html) {
  char tmp_tag[100];
  int i = 0;
  while (*html) {
    if (*html == '>') {
      char *tag = (char *) malloc(i+1);
      tmp_tag[i] = '\0';
      strcpy(tag, tmp_tag);
      Token token = { .type = START_TAG, .tag_name = tag };
      tokens[t_index] = token;
      t_index++;
      html++;
      return;
    }
    tmp_tag[i] = *html;
    i++;
    html++;
  }
}

// https://html.spec.whatwg.org/multipage/parsing.html#tokenization
// "The output of the tokenization step is a series of zero or more of the following tokens:
// DOCTYPE, start tag, end tag, comment, character, end-of-file"
void tokenize(char *html) {
  println(html);
  while (*html) {
    switch (*html) {
      case '<':
        html++;
        if (*html == '!') {
          html++;
          append_doctype(html);
        } else if (*html == '/') {
          html++;
          append_end_tag(html);
        } else {
          append_start_tag(html);
        }
        break;
      default:
        break;
    }
    html++;
  }
}

// for debug.
void print_tokens() {
  for (int i=0; i<t_index; i++) {
    println("token:");
    println(tokens[i].tag_name);
  }
}

void render(char* html) {
  nodei = 0;
  parse(html);
  generate();
}
