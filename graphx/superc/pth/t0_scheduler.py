
import os
from common import *

mem_per_exec = 2 # in GBs
node_count = 2
cores_per_node = 6

log_dir = prepare_log_dir()
log_prefix = os.path.join(log_dir, "graphx")

cmds = [graphx_test_cli(str(mem_per_exec) + "g", graph=std_g("p100k"), iterations=3, algo="colouring")]

os.system(run_batch_string(cmds,
                           node_count=node_count,
                           mem_per_node=str(mem_per_exec*cores_per_node+1) + "g",
                           cores_per_node=cores_per_node,
                           log_prefix=log_prefix,
                           time="00:50:00"))