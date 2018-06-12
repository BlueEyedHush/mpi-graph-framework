
import os
from common import *

# problem complexity - 1 process, changing problem size

build_type = "release"
mpt = "2gb"

def bench_set(nc, tpn, g_aliases, algo):
    cpus = nc*tpn
    div = max([1,cpus-1])

    for g_alias in g_aliases:
        log_dir = prepare_log_dir("{}_{}_{}".format(g_alias, nc, tpn))
        log_prefix = os.path.join(log_dir, "fr_" + build_type)
        cmds = [framework_cli(build_type, std_g(g_alias), algo, log_dir, vdiv=div, ediv=div)]

        jname = "fr_{}_{}_{}_{}_{}".format(build_type, g_alias, nc, tpn, mpt)
        os.system(run_batch_string(cmds,
                                   job_name=jname,
                                   queue="plgrid-short",
                                   mem_per_task=mpt,
                                   tasks_per_node=tpn,
                                   node_count=nc,
                                   log_prefix=log_prefix,
                                   profiling_on=False))

graph_vcounts = [5, 10, 15, 20, 25, 30]
bench_set(1, 1, map(lambda vc: "p{}k".format(vc), graph_vcounts), "colouring")
