#!/usr/bin/env bash

SPARK_HOME="/opt/spark/latest/"
SCRIPT_DIR="$(readlink -e $(dirname ${BASH_SOURCE[0]}))"
LOCAL_HISTORY_DIR="$SCRIPT_DIR/history_data/"

export SPARK_DAEMON_JAVA_OPTS="-Dspark.history.fs.logDirectory=file://$LOCAL_HISTORY_DIR"
"$SPARK_HOME"/sbin/start-history-server.sh