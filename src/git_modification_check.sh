#!/usr/bin/env bash

set -e

git add -N .
set +e
git diff --exit-code --quiet
if [[ $? -eq 1 ]]; then
	echo '(modified)'
else
	echo '(not modified)'
fi
