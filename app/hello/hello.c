#include <unistd.h>
int main() {
  write(1, "Hello, world!", 14);
  return 0;
}
