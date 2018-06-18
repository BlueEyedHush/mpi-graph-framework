
import os
from common import *

# scaling within single node

reps=3
mpt = "5gb"
g_alias, g_path = cst_g(2, 40)
algo = "colouring"
build_type = "release"

def bench_set(exec_specs):
    for nc, tpn in exec_specs:
        cpus = nc*tpn
        div = max([1,cpus-1])

        log_dir = prepare_log_dir("t1_{}_{}_{}".format(g_alias, nc, tpn))
        log_prefix = os.path.join(log_dir, "fr_" + build_type)
        cmds = framework_cli(build_type, g_path, algo, log_dir, vdiv=div, ediv=div, in_rs=1, in_rh=16, out=4, repetitions=3)

        jname = "fr_{}_{}_{}_{}_{}".format(build_type, g_alias, nc, tpn, mpt)
        os.system(run_batch_string(cmds,
                                   job_name=jname,
                                   time="00:30:00",
                                   queue="plgrid-testing",
                                   mem_per_task=mpt,
                                   tasks_per_node=tpn,
                                   node_count=nc,
                                   log_prefix=log_prefix,
                                   profiling_on=False))

bench_set(map(lambda x: (1,x), [1,2,4,6,8,10,12]))
