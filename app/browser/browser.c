#include "browser.h"
#include "rendering.h"

int main(int argc, char *argv[]) {
  if (argc != 2) {
    write(1, "usage: browser <rawtext>\n", 25);
    exit(1);
    return 1;
  }

  char* html = argv[1];
  render(html);

  exit(0);
  return 0;
}
