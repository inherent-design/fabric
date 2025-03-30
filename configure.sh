#!/usr/bin/env bash

set -e

if [[ -d "./build" ]]; then
  rm -rf ./build
fi

cmake -G Ninja \
  -B build
