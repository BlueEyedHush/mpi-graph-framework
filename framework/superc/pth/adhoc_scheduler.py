
import os
from common import *

nc = 1
tpn = 1
g_alias, g_path = "p50k", std_g("p50k") #cst_g(2, 40)
algo = "colouring"
build_type = "release"

log_dir = prepare_log_dir("{}_{}_{}".format(g_alias, nc, tpn))
log_prefix = os.path.join(log_dir, "fr_" + build_type)

cpus = nc*tpn
div = max([1,cpus-1])
cmds = framework_cli(build_type, g_path, algo, log_dir, vdiv=div, ediv=div)

os.system(run_batch_string(cmds,
                           job_name="adhoc_{}_{}_{}_{}".format(build_type, g_alias, nc, tpn),
                           queue="plgrid-testing",
                           mem_per_task="6gb",
                           tasks_per_node=tpn,
                           node_count=nc,
                           log_prefix=log_prefix,
                           profiling_on=True))
