
import os
from common import *

mem_per_node = "16g"
cores_per_node = 12
partitions_per_node = 2
g_alias, g_path = std_g("p100k")
algo = "colouring"

log_dir = prepare_log_dir("t0")

def schedule_job(node_count, iter):
    job_name = "{}_{}_{}_{}_i{}".format(node_count, mem_per_node, g_alias, algo, iter)

    log_prefix = os.path.join(log_dir, job_name)
    p_count = partitions_per_node*node_count
    cmds = [graphx_test_cli(mem_per_node,
                            graph=g_path,
                            iterations=1,
                            algo=algo,
                            verbose=False,
                            kryo=False,
                            partition_no=p_count)]

    os.system(run_batch_string(cmds,
                               job_name=job_name,
                               node_count=node_count,
                               mem_per_node=mem_per_node,
                               cores_per_node=cores_per_node,
                               log_prefix=log_prefix,
                               time="00:20:00"))

for nc in [1,4,8]:
    for iter in xrange(3):
        schedule_job(nc, iter)
