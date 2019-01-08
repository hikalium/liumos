#include "ring_buffer.h"

#ifdef LIUMOS_TEST

#include <stdio.h>
#include <cassert>

int main() {
  RingBuffer<int, 4> rbuf;

  rbuf.Push(3);
  rbuf.Push(5);
  rbuf.Push(7);
  rbuf.Push(11);
  rbuf.Push(13);
  assert(rbuf.Pop() == 3);
  rbuf.Push(17);
  assert(rbuf.Pop() == 5);
  assert(rbuf.Pop() == 7);
  assert(rbuf.Pop() == 17);
  assert(rbuf.Pop() == 0);

  puts("PASS");
  return 0;
}

#endif
