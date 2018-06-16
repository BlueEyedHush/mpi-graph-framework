#!/usr/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WD="$DIR"/data/
GENERATOR="$DIR"/er-gen/cmake-build-release/er_gen

pushd "$WD" > /dev/null

"$GENERATOR" $1 $2

popd > /dev/null