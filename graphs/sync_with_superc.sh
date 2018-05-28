#!/usr/bin/env bash

REL_DIR="$(dirname "${BASH_SOURCE[0]}")"
DIR="$(readlink -e $REL_DIR)"
MACHINE="prometheus.cyfronet.pl"
LOGIN_STR="plgblueeyedhush@$MACHINE"
REMOTE_DIR="ml-graphs/graphs/"

PASS=`cat $DIR/pass`

if [ ! -z "$1" ]; then
    echo "Creating directory"
    sshpass -p "$PASS" ssh "$LOGIN_STR" mkdir -p "$REMOTE_DIR"
fi

echo "Syncing files"
sshpass -p "$PASS" rsync -avzr --exclude pass "$DIR"/* "$LOGIN_STR":"$REMOTE_DIR"
