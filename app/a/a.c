typedef unsigned int size_t;
int write(int fd, const void*, size_t);
void exit(int);

int main() {
  while (1) {
    for (int i = 0; i < 10000000; i++) {
      asm volatile("pause");
    }
    write(1, "a\n", 2);
  }
}
