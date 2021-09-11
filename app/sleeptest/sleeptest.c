#include "../liumlib/liumlib.h"

int main(int argc, char** argv) {
  puts("sleeping 5 seconds...");
  struct timespec ts;
  ts.tv_sec = 5;
  ts.tv_nsec = 0;
  nanosleep(&ts, NULL);
  puts("done!");
  return 0;
}
