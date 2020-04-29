#include "command_line_args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cassert>

int main() {
  CommandLineArgs args;
  assert(args.GetNumOfArgs() == 0);
  assert(args.GetArg(-1) == nullptr);
  assert(args.GetArg(0) == nullptr);
  assert(args.GetArg(1) == nullptr);

  assert(args.Parse(""));
  assert(args.GetNumOfArgs() == 0);

  assert(args.Parse(" "));
  assert(args.GetNumOfArgs() == 0);

  assert(args.Parse("a"));
  assert(args.GetNumOfArgs() == 1);
  assert(args.GetArg(-1) == nullptr);
  assert(strcmp(args.GetArg(0), "a") == 0);
  assert(args.GetArg(1) == nullptr);

  assert(args.Parse(" a "));
  assert(args.GetNumOfArgs() == 1);
  assert(args.GetArg(-1) == nullptr);
  assert(strcmp(args.GetArg(0), "a") == 0);
  assert(args.GetArg(1) == nullptr);

  assert(args.Parse("a b"));
  assert(args.GetNumOfArgs() == 2);
  assert(args.GetArg(-1) == nullptr);
  assert(strcmp(args.GetArg(0), "a") == 0);
  assert(strcmp(args.GetArg(1), "b") == 0);
  assert(args.GetArg(2) == nullptr);

  assert(args.Parse("One Two  Three   Four "));
  assert(args.GetNumOfArgs() == 4);
  assert(args.GetArg(-1) == nullptr);
  assert(strcmp(args.GetArg(0), "One") == 0);
  assert(strcmp(args.GetArg(1), "Two") == 0);
  assert(strcmp(args.GetArg(2), "Three") == 0);
  assert(strcmp(args.GetArg(3), "Four") == 0);
  assert(args.GetArg(4) == nullptr);

  char buf[(CommandLineArgs::kMaxNumOfArgs + 1) * 2 + 1];
  for (int i = 0; i < CommandLineArgs::kMaxNumOfArgs; i++) {
    buf[i * 2 + 0] = '*';
    buf[i * 2 + 1] = ' ';
  }
  buf[CommandLineArgs::kMaxNumOfArgs * 2] = 0;
  assert(args.Parse(buf));
  assert(args.GetNumOfArgs() == CommandLineArgs::kMaxNumOfArgs);
  for (int i = 0; i < CommandLineArgs::kMaxNumOfArgs; i++) {
    assert(strcmp(args.GetArg(i), "*") == 0);
  }

  puts("PASS");
  return 0;
}
