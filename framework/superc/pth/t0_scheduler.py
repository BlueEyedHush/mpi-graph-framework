
import os
from common import *

log_dir = prepare_log_dir()

cmds = [framework_cli("release", "resources/main/complete50.adjl", "colouring")]

os.system(run_batch_string(cmds, tasks_per_node=3, log_prefix=log_dir))