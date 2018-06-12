#!/usr/bin/python

import argparse
import subprocess
import sys
import os

def err(msg):
    sys.stderr.write(str(msg))

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-bt', dest="build_type", default="Debug", choices=["Debug", "Release"])
    parser.add_argument('-r', dest="refresh_cmake", action="store_true", default=False)
    parser.add_argument('-v', dest="verbose", action="store_true", default=False)

    return parser.parse_args()

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
BASE_DIR = os.path.join(SCRIPT_DIR, "..", "..")

args = parse_args()
err(args)

cmds = [
    "module load plgrid/tools/cmake/3.10.2",
    "module load tools/impi/2018.1",
    "module load compilers/gcc/5.3.0"
]

build_dir = os.path.join(BASE_DIR, "cmake-build-{}".format(args.build_type.lower()))

cmds += ["mkdir -p {}".format(build_dir)]
cmds += ["pushd {} > /dev/null".format(build_dir)]

if args.refresh_cmake:
    cmds += ["cmake "
              "-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ "
              "-DCMAKE_BUILD_TYPE={} ../".format(args.build_type)]

verbose_str = "VERBOSE=1" if args.verbose else ""

cmds += ["make {} framework".format(verbose_str)]
cmds += ["popd > /dev/null"]

cmds_str = "\n".join(cmds)
err("\n\n" + cmds_str + "\n\n")
subprocess.call(cmds_str, shell=True)
