#!/usr/bin/env bash

set -e

if [[ ! -d "./build" ]]; then
  ./configure.sh
fi

pushd build
cmake --build .
popd
