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
    case TEXT:
      if (Printable(node->data)) {
        Print(node->data);
        first_line = false;
      }
      break;
    case HEADING:
      if (strcmp(node->tag_name, "h1") == 0) {
        Print("# ");
      } else if (strcmp(node->tag_name, "h2") == 0) {
        Print("## ");
      } else if (strcmp(node->tag_name, "h3") == 0) {
        Print("### ");
      } else if (strcmp(node->tag_name, "h4") == 0) {
        Print("#### ");
      } else if (strcmp(node->tag_name, "h5") == 0) {
        Print("##### ");
      } else if (strcmp(node->tag_name, "h6") == 0) {
        Print("###### ");
      }
      first_line = false;
      break;
    case UL:
      if (!first_line)
        Print("\n");
      break;
    case LI:
      if (!first_line)
        Print("\n");
      Print("- ");
      first_line = false;
      break;
    case DIV:
      if (!first_line)
        Print("\n");
      break;
    case PARAGRAPH:
      if (!first_line)
        Print("\n");
      break;
    default:
      break;
  }
}

void Dfs(Node *node) {
  Markdown(node);

  for (Node *cur = node->first_child; cur; cur = cur->next_sibling) {
    Dfs(cur);
  }
}

void Generate() {
  first_line = true;
  Dfs(root_node);
  Print("\n");
}

void Render(char* html) {
  Tokenize(html);
  //PrintTokens();

  ConstructTree();
  //PrintNodes();

  Generate();
}
