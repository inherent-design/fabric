#!/usr/bin/env bash

set -e

env CC=clang CXX=clang++ meson setup build
