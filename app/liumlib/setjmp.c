int _setjmp(long long *buf) {
  return __builtin_setjmp((void **)buf);
}

