#pragma once

template <int N>
class StringBuffer {
 public:
  void WriteChar(char c) {
    if (write_index_ >= N)
      return;
    buf_[write_index_++] = c;
  }
  void WriteString(const char* s) {
    while (*s) {
      WriteChar(*s);
      s++;
    }
  }
  void WriteHex8ZeroFilled(const uint8_t v) {
    for (int i = 1; i >= 0; i--) {
      char c = (v >> (4 * i)) & 0xF;
      if (c < 10)
        c += '0';
      else
        c += 'A' - 10;
      WriteChar(c);
    }
  }
  void WriteHex64(const uint64_t v) {
    int i;
    for (i = 15; i > 0; i--) {
      if ((v >> (4 * i)) & 0xF)
        break;
    }
    for (; i >= 0; i--) {
      char c = (v >> (4 * i)) & 0xF;
      if (c < 10)
        c += '0';
      else
        c += 'A' - 10;
      WriteChar(c);
    }
  }
  void WriteDecimal64(uint64_t value) {
    char s[20];
    int i = 0;
    for (; i < 20; i++) {
      s[i] = value % 10;
      value /= 10;
      if (!value)
        break;
    }
    for (; i >= 0; i--) {
      WriteChar('0' + s[i]);
    }
  }
  void WriteHex64ZeroFilled(const uint64_t v) {
    int i;
    for (i = 15; i >= 0; i--) {
      if (i == 11 || i == 7 || i == 3)
        WriteChar('\'');
      char c = (v >> (4 * i)) & 0xF;
      if (c < 10)
        c += '0';
      else
        c += 'A' - 10;
      WriteChar(c);
    }
  }
  const char* GetString() {
    buf_[write_index_] = 0;
    return buf_;
  }
  void Clear() { write_index_ = 0; }

 private:
  int write_index_ = 0;
  char buf_[N + 1];
};
