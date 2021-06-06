#!/usr/bin/env bash

set -e

printf `git rev-parse HEAD`

git add -N .
set +e
git diff --exit-code --quiet
if [[ $? -eq 1 ]]; then
	printf ' (modified)'
fi
