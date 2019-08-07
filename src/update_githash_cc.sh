#!/bin/bash
GITHASH_CONTENTS="const char *kGitHash = \"`git rev-parse HEAD | tr -d "\n"` `./git_modification_check.sh`\";"

if [ ! -f githash.cc ]
then
	echo "Create githash.cc"
	touch githash.cc
fi

if echo "${GITHASH_CONTENTS}" | cmp -s githash.cc -
then
	exit
fi
echo "Update githash.cc"
echo "${GITHASH_CONTENTS}" > githash.cc

