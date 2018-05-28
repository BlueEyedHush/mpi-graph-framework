#!/usr/bin/python

from common import *

# os.system might create new shell for each invocation, so I build full cli-string here
# and execute it as a whole
run_commands(import_modules_string() +
             start_spark_cluster() +
             ["pushd {} >> /dev/null".format(get_workdir())] +
             only_on_master(get_cmds()) +
             ["popd >> /dev/null"] +
             stop_spark_cluster())