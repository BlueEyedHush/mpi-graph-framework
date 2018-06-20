
import os
import sys
import datetime

mpiexec_prefix = "export I_MPI_PMI_LIBRARY=/net/slurm/releases/production/lib/libpmi.so; srun --label "
spark_history_dir = "/net/people/plgblueeyedhush/spark_history"

# -------------------
# Environment agnostic
# -------------------

def ensure_dir_exists(dir):
    os.system("mkdir -p " + dir)

def err(msg):
    sys.stderr.write(msg + "\n")

class Paths(object):
    def __init__(self):
        self.script_dir = os.path.dirname(os.path.realpath(sys.argv[0]))
        self.base_dir = self.script_dir
        self.log_dir = os.path.join(self.base_dir, "logs")

    def build_dir(self, type):
        return "cmake-build-{}".format(type)

    def abs_build_dir(self, type):
        return os.path.join(self.base_dir, self.build_dir(type))

cached_paths = None

def get_paths():
    if cached_paths is None:
        global cached_paths
        cached_paths = Paths()

    return cached_paths

def r(cmd):
    err('\n' + cmd + '\n')
    os.system(cmd)

# -------------------
# Meant for scheduler
# -------------------

def prepare_log_dir():
    p = get_paths()
    datetime_component = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    log_dir = os.path.join(p.log_dir, datetime_component)
    ensure_dir_exists(log_dir)
    return log_dir

def run_batch_string(cmds,
                     job_name = "graphx",
                     node_count = 1,
                     cores_per_node = 1,
                     mem_per_node = "1gb",
                     queue="plgrid-testing",
                     log_prefix="graphx",
                     time="00:20:00",
                     profiling_on=False):
    p = get_paths()
    script = os.path.join(p.script_dir, "executor.py")

    if profiling_on:
        profiler_cli = " --profile=task --acctg-freq 5 "
        cmds = cmds + ["sh5util -j #SLURM_JOB_ID -o {}.h5".format(log_prefix)]
    else:
        profiler_cli = ""

    cmds_arg_str = ' "' + '" "'.join(cmds) + '"'
    work_dir = ' "{}"'.format(p.base_dir)

    cmd = ("sbatch"
    " -J " + job_name +
    " -N " + str(node_count) +
    " --ntasks-per-node 1" +
    " --cpus-per-task " + str(cores_per_node) +
    " --mem " + mem_per_node +
    " --time " + time +
    " -A ccbmc6"
    " -p " + queue +
    " --output " + log_prefix + ".so"
    " --error " + log_prefix + ".se"
    " " + profiler_cli + script + work_dir + cmds_arg_str)

    print cmd
    return cmd

def graphx_test_cli(mem_per_executor, graph=None, algo=None, iterations=None, verbose=False):
    cli_args = ""
    if graph is not None:
        cli_args += " -g " + graph
    if algo is not None:
        cli_args += " -a " + algo
    if iterations is not None:
        cli_args += " -i {}".format(iterations)
    if verbose:
        cli_args += " -v"

    paths = get_paths()
    cmd = "#SPARK_HOME/bin/spark-submit " \
          "--master spark://#SPARK_MASTER_HOST:#SPARK_MASTER_PORT " \
          "--conf spark.executor.memory={} " \
          "--conf spark.eventLog.enabled=true " \
          "--conf spark.eventLog.dir=file:{} " \
          "--class perftest.ClusterRunner {}/graphx-perf-comp-assembly-*.jar {}"\
        .format(mem_per_executor, spark_history_dir, paths.base_dir, cli_args)

    return cmd


g_aliases = {
    "p1k": "powergraph_1000_9864",
    "p5k": "powergraph_5000_49816",
    "p10k": "powergraph_10000_99794",
    "p15k": "powergraph_15000_149784",
    "p20k": "powergraph_20000_199771",
    "p25k": "powergraph_25000_249765",
    "p30k": "powergraph_30000_299761",
    "p35k": "powergraph_35000_349755",
    "p40k": "powergraph_40000_399751",
    "p50k": "powergraph_50000_499745",
    "p60k": "powergraph_60000_599742",
    "p70k": "powergraph_70000_699736",
    "p80k": "powergraph_80000_799727",
    "p90k": "powergraph_90000_899721",
    "p100k": "powergraph_100000_999719",
    "p200k": "powergraph_200000_1999678",
    "p300k": "powergraph_300000_2999660",
    "p400k": "powergraph_400000_3999643",
    "p500k": "powergraph_500000_4999634"
}

def cst_g(v_count_m, e_m):
    return "c{}m{}".format(v_count_m, e_m), "../graphs/data/cst_{}000000_{}.elt".format(v_count_m, e_m)

def std_g(alias):
    return alias, "../graphs/data/{}.elt".format(g_aliases[alias])

# -------------------
# Meant for executor
# -------------------

def import_modules_string():
    return ["module load plgrid/apps/spark/2.0.1"]

def start_spark_cluster():
    return ["start-spark-cluster.sh"]

def stop_spark_cluster():
    return ["stop-spark-cluster.sh"]

def only_on_master(cmds):
    cmd = "; ".join(cmds)
    return ["if [ $SLURM_NODEID -eq 0 ]; then\n    {};\nfi".format(cmd)]

def run_commands(cmds):
    cmd = "\n".join(cmds)
    cmd = cmd.replace("#", "$")
    r(cmd)

def get_workdir():
    return sys.argv[1]

def get_cmds():
    return sys.argv[2:]