
import os
from common import *

mem_per_core = 2 # in GBs
node_count = 2
cores_per_node = 6
g_alias, g_path = std_g("p100k")
iter = 1
algo = "colouring"

mem_per_node = str(mem_per_core *cores_per_node+1) + "g"
job_name = "t0_{}_{}_{}_{}".format(node_count, mem_per_node, g_alias, algo)
log_dir = prepare_log_dir()
log_prefix = os.path.join(log_dir, job_name)

cmds = [graphx_test_cli(mem_per_node, graph=g_path, iterations=iter, algo=algo)]

os.system(run_batch_string(cmds,
                           job_name=job_name,
                           node_count=node_count,
                           mem_per_node=mem_per_node,
                           cores_per_node=cores_per_node,
                           log_prefix=log_prefix,
                           time="00:20:00"))