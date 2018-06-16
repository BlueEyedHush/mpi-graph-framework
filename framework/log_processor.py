
import sys
import re

def splitter(sentence, mapping):
    def append(node_id, line):
        if node_id not in mapping:
            mapping[node_id] = []
        mapping[node_id].append(line)

    m = re.match("([0-9])+?:", sentence)

    if m is not None:
        node_id = m.group(1)
        append(node_id, sentence)
    else:
        append("_", sentence)

    return mapping

def measurements_processor(node_to_log_map):
    probes = {}

    def add_measurement(type, measurement):
        if type not in probes:
            probes[type] = []

        probes[type].append(measurement)

    p_local_time = "Local times"
    p_global_time = "Global times"
    p_memory_frac = "Memory utilization"
    p_misc = "Miscellanous"

    for node, lines in node_to_log_map.iteritems():
        for line in lines:
            for m in re.finditer("\[P:(.+?):(.+?):(.+?)\]", line):
                probe_type = m.group(1).lower()
                probe_name = m.group(2)
                probe_value = m.group(3)

                if probe_type == "tl":
                    value_in_sec = float(probe_value)/1000000000
                    add_measurement(p_local_time,
                                    "[{}] {}: {}ns ({:.2f}s)".format(node, probe_name, probe_value, value_in_sec))
                elif probe_type == "tg":
                    value_in_sec = float(probe_value)/1000000000
                    add_measurement(p_global_time,
                                    "{}: {} ({:.2f}s)".format(probe_name, probe_value, value_in_sec))
                elif probe_type == "mf":
                    x, all = map(lambda x: int(x), probe_value.split("/"))
                    frac = float(all)/x
                    add_measurement(p_memory_frac,
                                    "[{}] {}: {} (1/{:.2f})".format(node, probe_name, probe_value, frac))
                else:
                    add_measurement(p_misc, "[{}] {}: {}".format(node, probe_name, probe_value))

    lines = []

    for type, measurements in probes.iteritems():
        lines.append("\n\n" + type + ":\n\n")
        lines.append("\n".join(measurements))

    node_to_log_map["probes"] = lines


if __name__ == "__main__":
    lines = sys.stdin.readlines()

    node_to_log_mapping = {}
    for line in lines:
        splitter(line, node_to_log_mapping)

    measurements_processor(node_to_log_mapping)

    node_to_str = {}
    for k in sorted(node_to_log_mapping.iterkeys()):
        node_to_str[k] = "".join(node_to_log_mapping[k])

    # print to separate files
    dir = "/tmp" if len(sys.argv) <= 1 else sys.argv[1]
    for k in node_to_str.iterkeys():
        f = open(dir + "/{}".format(k), "w")
        f.write(node_to_str[k])

    # print original to stdout
    for line in lines:
        sys.stdout.write(line)

