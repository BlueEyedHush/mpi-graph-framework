
import os
from common import *

log_dir = prepare_log_dir()
log_prefix = os.path.join(log_dir, "graphx")

cmds = [graphx_test_cli(graph="../graphs/data/powerlaw_25_2_05_876.elt", iterations=3, algo="colouring")]

os.system(run_batch_string(cmds, node_count=2, mem_per_task="2gb", log_prefix=log_prefix))