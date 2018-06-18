#!/usr/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
sbatch \
    -J gg_$1_$2  \
    -A ccbmc6 \
    -N 1 \
    --ntasks-per-node 1 \
    --mem-per-cpu 5g \
    --time 00:20:00 \
    -p plgrid-testing \
    "$DIR"/er_gen.sh $1 $2 "$DIR" \