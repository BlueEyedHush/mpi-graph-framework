#!/usr/bin/env bash

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"
MACHINE="prometheus.cyfronet.pl"

PASS=`cat $DIR/pass`
sshpass -p "$PASS" rsync -avzr \
    --exclude pass \
    --exclude CMakeFiles/ \
    --exclude CMakeCache.txt \
    --exclude .git/ \
    --exclude .idea/ \
    --exclude Testing/ \
    --exclude target/ \
    --exclude .gitignore \
    --exclude *.cbp \
    --exclude /*.a \
    --exclude Makefile \
    --exclude *.cmake \
    --exclude sync_with_superc.sh \
    "$DIR"/. plgblueeyedhush@$MACHINE:ml-graphs

