int counter;

extern "C" void KernelEntry() {
  for (;;) {
    counter++;
  }
}
