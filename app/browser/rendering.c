#include "rendering.h"

#include "tokenize.h"
#include "parse.h"
#include "lib.h"

/*
void generate() {
  for (int i=0; i<n_index; i++) {
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
  nodes[n_index] = create_node("text", text);
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
  nodes[n_index] = create_node(tag, "");
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
      n_index++;
    } else {
      i = parse_text(html, i);
      n_index++;
    }
  }
}
*/

void markdown(Node *node) {
  switch (node->element_type) {
    case TEXT:
      write(1, node->data, strlen(node->data));
      break;
    case HEADING:
      if (strcmp(node->local_name, "h1") == 0) {
        write(1, "# ", 2);
      } else if (strcmp(node->local_name, "h2") == 0) {
        write(1, "## ", 3);
      } else if (strcmp(node->local_name, "h3") == 0) {
        write(1, "### ", 4);
      } else if (strcmp(node->local_name, "h4") == 0) {
        write(1, "#### ", 5);
      } else if (strcmp(node->local_name, "h5") == 0) {
        write(1, "##### ", 6);
      } else if (strcmp(node->local_name, "h6") == 0) {
        write(1, "###### ", 7);
      }
      break;
    default:
      break;
  }
}

void generate() {
  Node *node = root_node;

  while (node) {
    markdown(node);

    Node *next = node->next;
    while (next) {
      markdown(next);
      next = next->next;
    }

    node = node->child;
  }

}

void render(char* html) {
  tokenize(html);
  //print_tokens();

  //println("================");

  construct_tree();
  //print_nodes();

  //println("================");

  generate();
  //write(1, "\n", 1);

  //parse(html);
  //generate();
}
