#!/bin/bash

assert() {
  expected="$1"
  input="$2"

  actual=$(./browser.bin --url_test "$input" 2>&1)

  if [ "$actual" = "$expected" ]; then
    echo "input: $input => result: $actual"
  else
    echo "input: $input => $expected expected, but got $actual"
    exit 1
  fi
}

make clean
make

assert "scheme: http
host: 127.0.0.1:8888
port: 8888
path: /index.html" "http://127.0.0.1:8888/index.html"
assert "scheme: http
host: 127.0.0.1
port: 80
path: /index.html" "http://127.0.0.1/index.html"
assert "scheme: http
host: 127.0.0.1
port: 80
path: /" "http://127.0.0.1"
assert "scheme: http
host: 127.0.0.1:8888
port: 8888
path: /" "http://127.0.0.1:8888"

echo "----------------"
echo "All tests passed"
echo "----------------"
