
import os
import sys
import datetime

mpiexec_prefix = "mpiexec -ordered-output -prepend-rank "

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

def prepare_log_dir():
    p = get_paths()
    datetime_component = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    log_dir = os.path.join(p.log_dir, datetime_component)
    ensure_dir_exists(log_dir)
    return log_dir

def run_batch_string(cmds,
                     node_count = 1,
                     tasks_per_node = 1,
                     mem_per_task = "1gb",
                     queue="plgrid-short",
                     log_prefix="framework",
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
    " -J framework"
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

def framework_cli(build_type, graph_file, assembly_name, log_dir, repetitions=1):
    paths = get_paths()
    framework_path = os.path.join(paths.build_dir(build_type), "framework")
    log_processor = "python {} {}".format(os.path.join(paths.base_dir, "log_processor.py"), log_dir)
    cmd = mpiexec_prefix + "{} -g {} -a repeating -ra-n {} -ra-name {} |& {}".format(
        framework_path,
        graph_file,
        repetitions,
        assembly_name,
        log_processor)
    return cmd


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
    cmd = "; ".join(cmds)
    cmd = cmd.replace("#", "$")
    err(cmd)
    os.system(cmd)

def get_workdir():
    return sys.argv[1]

def get_cmds():
    return sys.argv[2:]