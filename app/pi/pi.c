typedef unsigned int size_t;
int write(int fd, const void*, size_t);
void exit(int);

void printf04d(int v){
  char s[4];
  for(int i = 0; i < 4; i++){
    s[3-i] = v % 10 + '0';
    v /= 10;
  }
  write(1, s, 4);
}

int nume[52514];
int i;
int n;
int carry = 0;
int digit;
int base = 10000;
int denom;
int first = 0;

int main() {
  for (n = 52500; n > 0; n -= 14) {
    carry %= base;
    digit = carry;
    for (i = n - 1; i > 0; --i) {
      denom = 2 * i - 1;
      carry = carry * i + base * (first ? nume[i] : (base / 5));
      nume[i] = carry % denom;
      carry /= denom;
    }
    printf04d(digit + carry / base);
    first = 1;
  }
  write(1, "\n", 1);
  exit(0);
  return 0;
}
