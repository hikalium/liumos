typedef unsigned int size_t;
int write(int fd, const void *, size_t);
void exit(int);

int main() {
  write(1, "Hello, world!\n", 14);
  exit(0);
  return 0;
}
