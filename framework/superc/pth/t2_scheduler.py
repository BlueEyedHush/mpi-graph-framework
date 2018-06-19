
import os
from common import *

# problem complexity - 1 process, changing problem size

g_alias, g_path = cst_g(2, 40)
mpt = "5gb"
q = "plgrid-testing"
algo = "colouring"
reps = 3
tpn = 12
o_in = 1
o_out = 6
build_type = "release"
time = "00:20:00"

def bench_set(nc):
    cpus = nc*tpn
    div = max([1,cpus-1])

    log_dir = prepare_log_dir("t2_{}_{}_{}_{}".format(build_type, g_alias, nc, tpn))
    log_prefix = os.path.join(log_dir, "fr_" + build_type)
    cmds = framework_cli(build_type, g_path, algo, log_dir, vdiv=div, ediv=div, in_rs=o_in, in_rh=16*o_in, out=o_out, repetitions=reps)

    jname = "fr_{}_{}_{}_{}_{}".format(build_type, g_alias, nc, tpn, mpt)
    os.system(run_batch_string(cmds,
                               job_name=jname,
                               queue=q,
                               time=time,
                               mem_per_task=mpt,
                               tasks_per_node=tpn,
                               node_count=nc,
                               log_prefix=log_prefix,
                               profiling_on=False))

for nc in range(1,13):
    bench_set(nc)
