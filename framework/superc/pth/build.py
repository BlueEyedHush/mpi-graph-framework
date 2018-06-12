#!/usr/bin/python

import argparse
import os

def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-bt', dest="build_type", default="Release", choices=["Debug", "Release"])
    parser.add_argument('-r', dest="refresh_cmake", action="store_true", default=False)
    parser.add_argument('-v', dest="verbose", action="store_true", default=False)

    return parser.parse_args()

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
BASE_DIR = os.path.join(SCRIPT_DIR, "..", "..")

args = parse_args()
print args

os.system(
    "module load plgrid/tools/cmake/3.10.2"
    "module load tools/impi/2018.1"
    "module load compilers/gcc/5.3.0"
)

original_work_dir = os.path.abspath(os.getcwd())
build_dir = os.path.join(BASE_DIR, "cmake-build-{}".format(args.build_type.lower()))

os.system("mkdir -p".format(build_dir))
os.chdir(build_dir)

if args.refresh_cmake:
    os.system("cmake "
              "-DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ "
              "-DCMAKE_BUILD_TYPE={} ../".format(args.build_type))

verbose_str = "VERBOSE=1" if args.verbose else ""

os.system("make {} framework".format(verbose_str))

os.chdir(original_work_dir)