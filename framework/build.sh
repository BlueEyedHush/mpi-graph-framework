#!/usr/bin/env bash

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"/

OUT_DIR="$DIR"/target/
ARTIFACT="$OUT_DIR"framework

mkdir -p "$OUT_DIR"

shopt -s globstar
mpicc -cc=g++ -std=c++11 -ggdb3 -O0 -o "$ARTIFACT" "${DIR}"src/**/*.cpp
