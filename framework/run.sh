#!/usr/bin/env bash

VERTICES=4

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"/

ARTIFACT="$DIR"target/framework

mpiexec -n $VERTICES "$ARTIFACT" $@
