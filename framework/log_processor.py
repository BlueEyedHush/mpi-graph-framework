
import sys
import re

def splitter(sentence, mapping):
    def append(node_id, line):
        if node_id not in mapping:
            mapping[node_id] = []
        mapping[node_id].append(line)

    split = re.split("\[[0-9]+?\]", sentence)

    if len(split) > 1:
        groups = []
        for m in re.finditer("\[([0-9]+?)\]", sentence):
            groups.append(m.group(1))

        if split[0]:
            # if first is non-empty, line didn't start with node id indicator
            append("_", "{} (???)\n".format(split[0]))

        for node_id, str in zip(groups, split[1:]):
            if str[0] == ' ':
                str = str[1:]
            append(node_id, str)

    else:
        append("_", sentence)

    return mapping

def measurements_processor(node_to_log_map):
    local_time_probes = []
    global_time_probes = []
    misc_probes = []

    for node, lines in node_to_log_map.iteritems():
        for line in lines:
            for m in re.finditer("\[P:(.+?):(.+?):(.+?)\]", line):
                probe_type = m.group(1).lower()
                probe_name = m.group(2)
                probe_value = m.group(3)

                if probe_type == "tl":
                    value_in_sec = float(probe_value)/1000000000
                    local_time_probes.append("[{}] {}: {}ns ({}s)\n".format(node, probe_name, probe_value, value_in_sec))
                elif probe_type == "tg":
                    value_in_sec = float(probe_value)/1000000000
                    global_time_probes.append("{}: {} ({}s)\n".format(probe_name, probe_value, value_in_sec))
                elif probe_type == "mf":
                    x, all = map(lambda x: int(x), probe_value.split("/"))
                    frac = float(all)/x
                    misc_probes.append("[{}] {}: {} (1/{})".format(node, probe_name, probe_value, frac))
                else:
                    misc_probes.append("[{}] {}: {}\n".format(node, probe_name, probe_value))

    node_to_log_map["probes"] = ["Global times:\n"] + \
                                global_time_probes + \
                                ["\nLocal times:\n"] + \
                                local_time_probes + \
                                ["\nMisc probes\n"] + \
                                misc_probes


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

