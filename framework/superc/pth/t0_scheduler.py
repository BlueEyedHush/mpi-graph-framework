
import os
from common import *

log_dir = prepare_log_dir()
log_prefix = os.path.join(log_dir, "framework")

cmds = [framework_cli("debug", "../graphs/data/powergraph_1000_9864.adjl", "colouring", log_dir)]

os.system(run_batch_string(cmds,
                           queue="plgrid-testing",
                           tasks_per_node=3,
                           node_count=2,
                           log_prefix=log_prefix,
                           profiling_on=True))