#!/usr/bin/env bash

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"
MACHINE="prometheus.cyfronet.pl"

if [ ! -z "$1" ]; then
    CUSTOM_EXCLUDE="$1"
else
    CUSTOM_EXCLUDE="lks73892dskfja37t2299ekk"
fi

# rsync accepts first matching rule (whether it's include or exclude), so:
# 1. exclude unwanted directories
# 2. include all directories, since later on I want to include some stuff from lib/ (but not all!)
# 3. exclude rest from lib/
# 4. exclude specific files
rsync -avzr --prune-empty-dirs \
    --exclude "$CUSTOM_EXCLUDE" \
    --exclude CMakeFiles/ \
    --exclude .git/ \
    --exclude .idea/ \
    --exclude Testing/ \
    --exclude target/ \
    --include "*/" \
    --include "*.h" \
    --include "*.hpp" \
    --include "*.ipp" \
    --include "*.a" \
    --include "*.so" \
    --exclude "lib/**" \
    --exclude pass \
    --exclude CMakeCache.txt \
    --exclude .gitignore \
    --exclude *.cbp \
    --exclude /*.a \
    --exclude Makefile \
    --exclude *.cmake \
    --exclude sync_with_superc.sh \
    "$DIR"/. plgblueeyedhush@$MACHINE:ml-graphs/framework

