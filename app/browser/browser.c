#include "browser.h"
#include "rendering.h"

//#include <stdio.h>

int main(int argc, char *argv[]) {
  if (argc != 2) {
    write(1, "usage: browser <rawtext>\n", 25);
    exit(1);
    return 1;
  }

  char* html = argv[1];
  render(html);

  /*
  char output[1000];
  int i = 0;
  while (md[i] != '\0') {
    output[i] = md[i];
    i++;
  }
  output[i] = '\n';
  //write(1, output, i+1);
  */

  exit(0);
  return 0;
}
