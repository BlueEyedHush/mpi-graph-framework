#!/usr/bin/env bash

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"/

OUT_DIR="$DIR"/target/
ARTIFACT="$OUT_DIR"framework

mkdir -p "$OUT_DIR"

mpicc -cc=gcc -std=c11 -o "$ARTIFACT" "$DIR"main.c
