#ifndef APP_BROWSER_RENDERING_H
#define APP_BROWSER_RENDERING_H

// This parsing model for HTML is a subset of the parsing model in the spec.
// https://html.spec.whatwg.org/multipage/parsing.html

/*
typedef enum Tag {
  HTML,
  BODY,
  H1,
  H2,
  H3,
  H4,
  H5,
  H6,
  UL,
  LI,
  TEXT,
} Tag;

typedef struct Node {
  Tag tag;
  char *text;
} Node;

void generate();
void parse(char *html);
*/
void render(char *html);

/*
int n_index = 0;
struct Node nodes[100];
*/

#endif // APP_BROWSER_RENDERING_H
