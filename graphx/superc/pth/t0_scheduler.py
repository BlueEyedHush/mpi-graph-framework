
import os
from common import *

mem_per_exec = 2 # in GBs
node_count = 2
cores_per_node = 6
g_alias, g_path = std_g("p100k")
iter = 1
algo = "colouring"

job_name = "t0_{}_{}g_{}_{}".format(node_count*cores_per_node, mem_per_exec*node_count, g_alias, algo)
log_dir = prepare_log_dir()
log_prefix = os.path.join(log_dir, "graphx")

cmds = [graphx_test_cli(str(mem_per_exec) + "g", graph=g_path, iterations=iter, algo=algo)]

os.system(run_batch_string(cmds,
                           job_name=job_name,
                           node_count=node_count,
                           mem_per_node=str(mem_per_exec*cores_per_node+1) + "g",
                           cores_per_node=cores_per_node,
                           log_prefix=log_prefix,
                           time="00:20:00"))