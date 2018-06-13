
import os
from common import *

# problem complexity - 1 process, changing problem size

build_type = "release"
mpt = "5gb"

def bench_set(exec_specs, g_alias, algo):
    for nc, tpn in exec_specs:
        cpus = nc*tpn
        div = max([1,cpus-1])

        log_dir = prepare_log_dir("t1_{}_{}_{}".format(g_alias, nc, tpn))
        log_prefix = os.path.join(log_dir, "fr_" + build_type)
        cmds = framework_cli(build_type, std_g(g_alias), algo, log_dir, vdiv=div, ediv=div)

        jname = "fr_{}_{}_{}_{}_{}".format(build_type, g_alias, nc, tpn, mpt)
        os.system(run_batch_string(cmds,
                                   job_name=jname,
                                   queue="plgrid-short",
                                   mem_per_task=mpt,
                                   tasks_per_node=tpn,
                                   node_count=nc,
                                   log_prefix=log_prefix,
                                   profiling_on=False))

exec_specs = [(1,1)]
bench_set(exec_specs, "p500k", "colouring")
