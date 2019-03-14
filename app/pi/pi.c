typedef unsigned int size_t;
int write(int fd, const void*, size_t);
void exit(int);

int base = 10000;
int n = 8400;
int i;
int temp;
int out;
int denom;
int numerator[8401];

void printf04d(int v){
  char s[4];
  for(int i = 0; i < 4; i++){
    s[3-i] = v % 10 + '0';
    v /= 10;
  }
  write(1, s, 4);
}

int main(void) {
  base = 10000;
  n = 8400;
  for (i = 0; i < n; i++) {
    numerator[i] = base / 5;
  }
  out = 0;
  for (n = 8400; n > 0; n -= 14) {
    temp = 0;
    for (i = n - 1; i > 0; i--) {
      denom = 2 * i - 1;
      temp = temp * i + numerator[i] * base;
      numerator[i] = temp % denom;
      temp /= denom;
    }
    printf04d(out + temp / base);
    out = temp % base;
  }
  exit(0);
  return 0;
}
