
import os
from common import *

log_dir = prepare_log_dir()
log_prefix = os.path.join(log_dir, "graphx")

cmds = [graphx_test_cli(log_dir)]

os.system(run_batch_string(cmds, tasks_per_node=3, log_prefix=log_prefix))