#include "rendering.h"

#include "tokenize.h"
#include "parse.h"

bool Printable(char *data) {
  bool printable = false;
  while (*data) {
    if (*data != 0x09 /* Tab */ &&
       *data != 0x0a /* LF */ &&
       *data != 0x0c /* FF */ &&
       *data != 0x0d /* CR */ &&
       *data != 0x20 /* Space */) {
      return true;
    }
    data++;
  }
  return printable;
}

void Markdown(Node *node) {
  switch (node->element_type) {
    case TEXT: if (Printable(node->data))
        Print(node->data);
      break;
    case HEADING:
      if (strcmp(node->local_name, "h1") == 0) {
        Print("# ");
      } else if (strcmp(node->local_name, "h2") == 0) {
        Print("## ");
      } else if (strcmp(node->local_name, "h3") == 0) {
        Print("### ");
      } else if (strcmp(node->local_name, "h4") == 0) {
        Print("#### ");
      } else if (strcmp(node->local_name, "h5") == 0) {
        Print("##### ");
      } else if (strcmp(node->local_name, "h6") == 0) {
        Print("###### ");
      }
      break;
    case UL:
      Print("\n");
      break;
    case LI:
      Print("\n");
      Print("- ");
      break;
    case DIV:
      Print("\n");
      break;
    case PARAGRAPH:
      Print("\n");
      break;
    default:
      break;
  }
}

void Generate() {
  Node *node = root_node;

  while (node) {
    Markdown(node);

    Node *next = node->next;
    while (next) {
      Markdown(next);
      next = next->next;
    }

    node = node->first_child;
  }
  Print("\n");
}

void Render(char* html) {
  Tokenize(html);
  //PrintTokens();

  ConstructTree();
  //PrintNodes();

  Generate();
}
