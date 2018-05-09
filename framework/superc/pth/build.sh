#!/usr/bin/env bash

module load plgrid/tools/cmake/3.10.2
module load tools/impi/2018.1

pushd "$HOME"/ml-graphs/  > /dev/null

mkdir -p CMakeFiles
pushd CMakeFiles  > /dev/null

cmake -DCMAKE_C_COMPILER=icc -DCMAKE_CXX_COMPILER=icpc -DCMAKE_BUILD_TYPE=Release ../
if [ -z "$1" ]; then
    make all
else
    make "$1"
fi

popd  > /dev/null
popd  > /dev/null