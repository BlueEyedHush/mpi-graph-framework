
import os
from common import *

nc = 1
tpn = 1
g_alias = "p35k"
algo = "colouring"
build_type = "debug"

log_dir = prepare_log_dir("{}_{}_{}".format(g_alias, nc, tpn))
log_prefix = os.path.join(log_dir, "fr_" + build_type)

cpus = nc*tpn
div = max([1,cpus-1])
cmds = [framework_cli(build_type, std_g(g_alias), algo, log_dir, vdiv=div, ediv=div)]

os.system(run_batch_string(cmds,
                           job_name="adhoc_{}_{}_{}_{}".format(build_type, g_alias, nc, tpn),
                           queue="plgrid-testing",
                           mem_per_task="5gb",
                           tasks_per_node=tpn,
                           node_count=nc,
                           log_prefix=log_prefix,
                           profiling_on=False))
