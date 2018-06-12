
import os
from common import *

nc = 2
tpn = 3
g_alias = "p50k"
algo = "colouring"
build_type = "release"

log_dir = prepare_log_dir("{}_{}_{}".format(g_alias, nc, tpn))
log_prefix = os.path.join(log_dir, "fr_" + build_type)

cpus = nc*tpn
div = max([1,cpus-1])
cmds = [framework_cli(build_type, std_g(g_alias), algo, log_dir, vdiv=div, ediv=div)]

os.system(run_batch_string(cmds,
                           queue="plgrid-testing",
                           tasks_per_node=tpn,
                           node_count=nc,
                           log_prefix=log_prefix,
                           profiling_on=False))
