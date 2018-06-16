#!/usr/bin/bash
# SBATCH -A ccbmc6
# SBATCH -N 1
# SBATCH --ntasks-per-node 1
# SBATCH --mem-per-cpu 1g
# SBATCH --time 00:20:00
# SBATCH -J gg
# SBATCH -p plgrid-testing

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
WD="$DIR"/data/
GENERATOR="$DIR"/er-gen/cmake-build-release/er_gen

pushd "$WD" > /dev/null

"$GENERATOR" $1 $2

popd > /dev/null