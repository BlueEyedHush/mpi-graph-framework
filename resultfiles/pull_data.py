#!/usr/bin/python

import sys
import os

remote_root="~/ml-graphs/"

def remote_log_dir(project):
    return "{}{}/logs/".format(remote_root, project)

if len(sys.argv) < 2:
    raise Exception("Missing argument: data source (graphx|framework)")
else:
    data_src = sys.argv[1]

script_dir = os.path.dirname(os.path.realpath(__file__))
target_dir = os.path.join(script_dir, data_src)

pull_cmd = "mkdir -p {}; rsync -rv 'plgblueeyedhush@prometheus.cyfronet.pl:{}' {}/"\
    .format(target_dir, remote_log_dir(data_src), target_dir)

print pull_cmd
os.system(pull_cmd)