#!/usr/bin/env bash

BUILD_TYPE=Release

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

module load plgrid/tools/cmake/3.10.2
module load tools/impi/2018.1
module load compilers/gcc/5.3.0

pushd "$DIR"/../../  > /dev/null

LOWERCASE_BT=`echo ${BUILD_TYPE,,}`
BT_SPECIFIC_DIR=cmake-build-$LOWERCASE_BT
mkdir -p $BT_SPECIFIC_DIR

pushd $BT_SPECIFIC_DIR  > /dev/null

if [ "$1" = "refresh" ] || [ ! -f CMakeCache.txt ]; then
    cmake -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=$BUILD_TYPE ../
fi

make all

popd  > /dev/null
popd  > /dev/null