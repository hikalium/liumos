void exit(int);
void write(int, const char *, int);

void HelloFromC(const char *s) {
  write(1, s, 1);
  exit(17);
  return;
}
