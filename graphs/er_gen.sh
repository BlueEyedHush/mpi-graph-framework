#!/usr/bin/bash

# can't do it like this, slurm
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ ! -z "$3" ]; then
    # for sbatch we have to set workdir manually, because slurm copies script
    DIR="$3"
fi

WD="$DIR"/data/
GENERATOR="$DIR"/er-gen/cmake-build-release/er_gen

pushd "$WD" > /dev/null

"$GENERATOR" $1 $2

popd > /dev/null