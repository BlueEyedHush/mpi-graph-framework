#!/usr/bin/env bash

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"/

OUT_DIR="$DIR"/target/
ARTIFACT="$OUT_DIR"framework

mkdir -p "$OUT_DIR"

mpicc -cc=g++ -std=c++11 -o "$ARTIFACT" "$DIR"main.cpp
