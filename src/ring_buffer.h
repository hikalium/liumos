#pragma once
#include <cstdint>

template <typename T, unsigned int n>
class RingBuffer {
 public:
  RingBuffer() : readp_(0), writep_(0) {}
  T Pop(void) {
    if (readp_ == writep_)
      return 0;
    T v = elements_[readp_];
    readp_ = (readp_ + 1) % n;
    return v;
  }
  void Push(T value) {
    int nextp = (writep_ + 1) % n;
    if (nextp == readp_)
      return;
    elements_[writep_] = value;
    writep_ = nextp;
  }

 private:
  T elements_[n];
  int readp_;
  int writep_;
};
