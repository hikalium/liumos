#include "browser.h"
#include "rendering.h"
#include "malloc.h"

int nodei;
struct Node nodes[100];

void write_text(char* text) {
  char output[100000];
  int i = 0;
  while (text[i] != '\0') {
    output[i] = text[i];
    i++;
  }
  write(1, output, i+1);
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
        write_text(nodes[i].text);
        break;
      case HTML:
      case BODY:
      case UL:
      default:
        break;
    }
  }
  write(1, "\n", 1);
}

// Return 0 if two strings are identical.
int my_strncmp(char* str1, char* str2, unsigned int n) {
  for (int i=0; i<n; i++) {
    if (str1[i] == '\0' || str2[i] == '\0')
      return 0;
    if (str1[i] != str2[i])
      return 1;
  }
  return 0;
}

Node create_node(char* tag_text, char* text) {
  Tag tag;
  if (my_strncmp(tag_text, "html", 4) == 0) {
    tag = HTML;
  } else if (my_strncmp(tag_text, "body", 4) == 0) {
    tag = BODY;
  } else if (my_strncmp(tag_text, "h1", 2) == 0) {
    tag = H1;
  } else if (my_strncmp(tag_text, "h2", 2) == 0) {
    tag = H2;
  } else if (my_strncmp(tag_text, "h3", 2) == 0) {
    tag = H3;
  } else if (my_strncmp(tag_text, "h4", 2) == 0) {
    tag = H4;
  } else if (my_strncmp(tag_text, "h5", 2) == 0) {
    tag = H5;
  } else if (my_strncmp(tag_text, "h6", 2) == 0) {
    tag = H6;
  } else if (my_strncmp(tag_text, "ul", 2) == 0) {
    tag = UL;
  } else if (my_strncmp(tag_text, "li", 2) == 0) {
    tag = LI;
  } else if (my_strncmp(tag_text, "text", 4) == 0) {
    tag = TEXT;
  }
  Node node = {tag, text};
  return node;
}

int parse_text(char* html, int i) {
  char* text = (char *) malloc(1000);
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

int skip_closing_tag(char* html, int i) {
  while (html[i] != '>') {
    i++;
  }
  // skip '>' and return the next index.
  return ++i;
}

int parse_element(char* html, int i) {
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

void parse(char* html) {
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

void render(char* html) {
  nodei = 0;
  parse(html);
  generate();
}
