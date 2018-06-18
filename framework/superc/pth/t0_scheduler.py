
import os
from common import *

# problem complexity - 1 process, changing problem size

mpt = "5gb"
nc = 1
q = "plgrid-testing"
algo = "colouring"
reps = 3

def bench_set(build_type, tpn, g_alias, o_in, o_out):
    cpus = nc*tpn
    div = max([1,cpus-1])

    log_dir = prepare_log_dir("t0_{}_{}_{}_{}".format(bt, g_alias, nc, tpn))
    log_prefix = os.path.join(log_dir, "fr_" + build_type)
    cmds = framework_cli(build_type, std_g(g_alias), algo, log_dir, vdiv=div, ediv=div, in_rs=o_in, in_rh=16*o_in, out=o_out, repetitions=reps)

    jname = "fr_{}_{}_{}_{}_{}".format(build_type, g_alias, nc, tpn, mpt)
    os.system(run_batch_string(cmds,
                               job_name=jname,
                               queue=q,
                               mem_per_task=mpt,
                               tasks_per_node=tpn,
                               node_count=nc,
                               log_prefix=log_prefix,
                               profiling_on=False))

graph_vcounts = [100, 200, 300, 400]
run_confs = [("release", 1, (1, 4)), ("release", 3, (1,4)), ("nolocal", 1, (3, 40))]

for gvc in graph_vcounts:
    for bt, tpn, (o_in, o_out) in run_confs:
        bench_set(bt, tpn, "p{}k".format(gvc), o_in, o_out)

