#!/bin/bash -e

assert() {
  input="$1"
  expected="$2"

  actual=`./argstest.bin $input`

  if [ "$actual" = "$expected" ]; then
    echo "PASS ./argstest.bin $input "
  else
    printf "FAIL ./argstest.bin $input => \n%s\n---- above expected, but got:\n%s\n" \
			"${expected}" "${actual}"
    exit 1
  fi
}

assert "" "`cat << EOS
argc: 1
argv[0]: ./argstest.bin
EOS
`"

assert "one" "`cat << EOS
argc: 2
argv[0]: ./argstest.bin
argv[1]: one
EOS
`"

assert "one two" "`cat << EOS
argc: 3
argv[0]: ./argstest.bin
argv[1]: one
argv[2]: two
EOS
`"

assert "one two three" "`cat << EOS
argc: 4
argv[0]: ./argstest.bin
argv[1]: one
argv[2]: two
argv[3]: three
EOS
`"


echo "All tests passed"
