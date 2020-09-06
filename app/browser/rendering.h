#ifndef RENDERING_H 
#define RENDERING_H 

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
  enum Tag tag;
  char* text;
} Node;

void generate();
void parse(char* html);
void render(char* html);

#endif // RENDERING_H
