#!/usr/bin/env bash

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"
MACHINE="prometheus.cyfronet.pl"

if [ ! -z "$1" ]; then
    CUSTOM_EXCLUDE="$1"
else
    CUSTOM_EXCLUDE="lks73892dskfja37t2299ekk"
fi

rsync -avzr \
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
    --exclude "$CUSTOM_EXCLUDE" \
    "$DIR"/. plgblueeyedhush@$MACHINE:ml-graphs/framework

