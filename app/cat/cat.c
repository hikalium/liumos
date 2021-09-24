#include "../liumlib/liumlib.h"

int main(int argc, char** argv) {
  // Concat
  if (argc < 2) {
    Print("Usage: ");
    Print(argv[0]);
    Print(" <file> ...\n");
    exit(EXIT_FAILURE);
  }
  int fd = open(argv[1], 0, 0);
  if(fd < 0) {
    Print("Failed to open a file\n");
    exit(EXIT_FAILURE);
  }
  char buf[1];
  while(read(fd, buf, sizeof(buf)) == 1) {
    putchar(buf[0]); 
  }
}
