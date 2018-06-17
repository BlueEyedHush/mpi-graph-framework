
import os
from common import *

log_dir = prepare_log_dir()
log_prefix = os.path.join(log_dir, "graphx")

cmds = [graphx_test_cli(graph="../graphs/data/powergraph_100000_999719.elt", iterations=3, algo="colouring")]

os.system(run_batch_string(cmds, node_count=2, mem_per_task="4gb", tasks_per_node=2, log_prefix=log_prefix, time="00:50:00"))