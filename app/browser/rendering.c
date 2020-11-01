#include "rendering.h"

#include "tokenize.h"
#include "parse.h"

void Markdown(Node *node) {
  switch (node->element_type) {
    case TEXT:
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
    case LI:
      Print("\n- ");
      break;
    case DIV:
      Print("\n");
    case PARAGRAPH:
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
}

void Render(char* html) {
  Tokenize(html);
  //PrintTokens();

  ConstructTree();
  //PrintNodes();

  Generate();
  //Print("\n");
}
