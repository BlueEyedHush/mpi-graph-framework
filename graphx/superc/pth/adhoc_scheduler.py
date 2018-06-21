
import os
from common import *

mem_per_node = "48g"
node_count = 1
cores_per_node = 12
g_alias, g_path = std_g("p100k")
iter = 1
algo = "colouring"

job_name = "t0_{}_{}_{}_{}".format(node_count, mem_per_node, g_alias, algo)
log_dir = prepare_log_dir("adhoc")
log_prefix = os.path.join(log_dir, job_name)
p_count = 2*node_count*cores_per_node

cmds = [graphx_test_cli(mem_per_node, graph=g_path, iterations=iter, algo=algo, verbose=True, kryo=False, partition_no=p_count)]

os.system(run_batch_string(cmds,
                           job_name=job_name,
                           node_count=node_count,
                           mem_per_node=mem_per_node,
                           cores_per_node=cores_per_node,
                           log_prefix=log_prefix,
                           time="00:20:00"))