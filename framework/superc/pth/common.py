
import os
import sys
import datetime

# mpiexec_prefix = "mpiexec -ordered-output -prepend-rank -genv I_MPI_DEBUG=100 -genv I_MPI_HYDRA_DEBUG=1 "
mpiexec_prefix = "export I_MPI_PMI_LIBRARY=/net/slurm/releases/production/lib/libpmi.so; srun --label "

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
        self.base_dir = os.path.abspath(os.path.join(self.script_dir, "..", ".."))
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

# -------------------
# Meant for scheduler
# -------------------

def prepare_log_dir(date_prefix=""):
    p = get_paths()
    datetime_component = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    date_prefix = (date_prefix + "_") if len(date_prefix) > 0 else date_prefix
    log_dir = os.path.join(p.log_dir, date_prefix + datetime_component)
    ensure_dir_exists(log_dir)
    return log_dir

def run_batch_string(cmds,
                     job_name="framework",
                     node_count = 1,
                     tasks_per_node = 1,
                     mem_per_task = "1gb",
                     queue="plgrid-short",
                     log_prefix="framework",
                     time="00:10:00",
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
    " --ntasks-per-node " + str(tasks_per_node) +
    " --mem-per-cpu " + mem_per_task +
    " --time " + time +
    " -A ccbmc6"
    " -p " + queue +
    " --output " + log_prefix + ".so"
    " --error " + log_prefix + ".se"
    " --mail-type=END,FAIL"
    " --mail-user=knawara112@gmail.com " + profiler_cli + script + work_dir + cmds_arg_str)

    print cmd
    return cmd

def framework_cli(build_type, graph_file, assembly_name, log_dir, repetitions=1, vdiv=1, ediv=1, in_rs=1, in_rh=2, out="a"):
    paths = get_paths()
    framework_path = os.path.join(paths.build_dir(build_type), "framework")

    create_tmp_file_cmd = "TMP_FILE=`mktemp #SCRATCH/fr_out.XXXXXXXX`"

    assembly_conf = "-a repeating -ra-n {} -ra-name {}".format(repetitions, assembly_name)
    div_conf = "-vdiv {} -ediv {}".format(vdiv, ediv)
    gcm_mem = "-gcm-in-rs {} -gcm-in-rh {} -gcm-out {}".format(in_rs, in_rh, out)

    framework_cmd = mpiexec_prefix + "{} -g {} {} {} {} |& tee #TMP_FILE".format(
        framework_path,
        graph_file,
        assembly_conf,
        div_conf,
        gcm_mem)

    log_processor_cmd = "cat #TMP_FILE | python {} {}".format(os.path.join(paths.base_dir, "log_processor.py"), log_dir)

    return [create_tmp_file_cmd, framework_cmd, log_processor_cmd]

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
    return "c{}m{}".format(v_count_m, e_m), "../graphs/data/cst_{}000000_{}.adjl".format(v_count_m, e_m)

def std_g(alias):
    return "../graphs/data/{}.adjl".format(g_aliases[alias])



# -------------------
# Meant for executor
# -------------------

def import_modules_string():
    return [
        "module load plgrid/tools/cmake/3.10.2",
        "module load tools/impi/2018.1",
        "module load compilers/gcc/5.3.0"
    ]

def run_commands(cmds):
    cmd = "\n".join(cmds)
    cmd = cmd.replace("#", "$")
    err("\n" + cmd + "\n")
    os.system(cmd)

def get_workdir():
    return sys.argv[1]

def get_cmds():
    return sys.argv[2:]