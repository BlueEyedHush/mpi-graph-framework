#!/usr/bin/env bash

SCRIPT_DIR="$(readlink -e $(dirname ${BASH_SOURCE[0]}))"
REMOTE_DIR="/net/people/plgblueeyedhush/spark_history/"

LOCAL_HISTORY_DIR="$SCRIPT_DIR/history_data/"
LOGS_DIR="$SCRIPT_DIR"/logs/

mkdir -p "$LOCAL_HISTORY_DIR"
mkdir -p "$LOGS_DIR"

rsync -rv --progress plgblueeyedhush@prometheus.cyfronet.pl:"$REMOTE_DIR" "$LOCAL_HISTORY_DIR"


