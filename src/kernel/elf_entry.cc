int counter;

extern "C" void ELFEntry() {
  for (;;) {
    counter++;
  }
}
