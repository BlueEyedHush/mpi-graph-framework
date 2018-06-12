
import os
from common import *

log_dir = prepare_log_dir()
log_prefix = os.path.join(log_dir, "framework")

nc = 2
tpn = 3

cpus = nc*tpn
cmds = [framework_cli("debug", std_g("p10k"), "colouring", log_dir, vdiv=cpus-1, ediv=cpus-1)]

os.system(run_batch_string(cmds,
                           queue="plgrid-testing",
                           tasks_per_node=tpn,
                           node_count=nc,
                           log_prefix=log_prefix,
                           profiling_on=True))
