#!/usr/bin/env bash

if [ -z "$1" ]; then
    echo "usage: splitOutputs.sh <processNum>"
    exit 1
else
    processesCount="$1"
fi

lastProcId=$((processesCount-1))
for procId in `seq 0 ${lastProcId}`; do
    cat /tmp/log | grep \\[${procId}\\] > /tmp/proc${procId}
done