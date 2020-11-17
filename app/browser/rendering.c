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

bool Markdown(Node *node, bool first_line) {
  switch (node->element_type) {
    case TEXT: if (Printable(node->data))
        Print(node->data);
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
      break;
    case UL:
      if (first_line)
        return true;
      Print("\n");
      break;
    case LI:
      if (!first_line)
        Print("\n");
      Print("- ");
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
      if (first_line)
        return true;
  }
  return false;
}

void Dfs(Node *node, bool first_line) {
  if (node == NULL)
    return;

  first_line = Markdown(node, first_line);

  Dfs(node->first_child, first_line);

  Node *next = node->next_sibling;
  while (next) {
    Dfs(next, first_line);
    next = next->next_sibling;
  }
}

void Generate() {
  Dfs(root_node, true);
  Print("\n");
}

void Render(char* html) {
  Tokenize(html);
  //PrintTokens();

  ConstructTree();
  //PrintNodes();

  Generate();
}
