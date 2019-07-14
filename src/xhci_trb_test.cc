#include "xhci_trb.h"

#ifdef LIUMOS_TEST

#include <stdio.h>
#include <cassert>

template <int N>
void TestTransferRequestBlock() {
  printf("Testing TransferRequestBlock<%d>...\n", N);
  TransferRequestBlock<N> trb;
  trb.Init();
  for (int i = 0; i < N; i++) {
    assert(trb.GetNextEnqueueIndex() == i);
    assert(reinterpret_cast<uint64_t>(&trb) + 16 * i ==
           trb.template GetNextEnqueueEntry<uint64_t>());
    assert(trb.GetCurrentCycleState() == 0);
    assert(trb.Push() == 0);
  }
  for (int i = 0; i < N; i++) {
    assert(trb.GetNextEnqueueIndex() == i);
    assert(reinterpret_cast<uint64_t>(&trb) + 16 * i ==
           trb.template GetNextEnqueueEntry<uint64_t>());
    assert(trb.GetCurrentCycleState() == 1);
    assert(trb.Push() == 1);
  }
  assert(trb.GetNextEnqueueIndex() == 0);
  assert(trb.GetCurrentCycleState() == 0);
}

int main() {
  TestTransferRequestBlock<256>();
  TestTransferRequestBlock<17>();
  TestTransferRequestBlock<1>();

  puts("PASS");
  return 0;
}

#endif
