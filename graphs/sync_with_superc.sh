#!/usr/bin/env bash

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"
MACHINE="prometheus.cyfronet.pl"
LOGIN_STR="plgblueeyedhush@$MACHINE"
REMOTE_DIR="ml-graphs/graphs/"

if [ ! -z "$1" ]; then
    echo "Creating directory"
    ssh "$LOGIN_STR" mkdir -p "$REMOTE_DIR"
fi

echo "Syncing files"
rsync -avzr --exclude pass \
    --exclude data/ \
    --exclude er-gen/cmake-build-debug \
    --exclude er-gen/cmake-build-release \
    --exclude er-gen/.idea/ \
    "$DIR"/* "$LOGIN_STR":"$REMOTE_DIR"
