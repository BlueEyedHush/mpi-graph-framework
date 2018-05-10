#!/usr/bin/python

from common import *

workdir = get_workdir()
cmds = get_cmds()

run_commands(import_modules_string() +
             ["pushd {}".format(workdir)] +
             cmds +
             ["popd"])