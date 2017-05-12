#!/usr/bin/env bash

NODES=2

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"/

ARTIFACT="$DIR"/framework

mpiexec -n $NODES "$ARTIFACT" $@
