
import sys
import re

def splitter(sentence, mapping):
    split = re.split("\[[0-9]+?\]", sentence)


    if len(split) > 1:
        groups = []
        for m in re.finditer("\[([0-9]+?)\]", sentence):
            groups.append(m.group(1))

        if split[0]:
            # if first is non-empty, line didn't start with node id indicator
            mapping["_"].append(split[0])
            mapping["_"].append(" (???) \n")

        for node_id, str in zip(groups, split[1:]):
            if node_id not in mapping:
                mapping[node_id] = []

            if str[0] == ' ':
                str = str[1:]

            mapping[node_id].append(str)

    else:
        mapping["_"].append(sentence)

    return mapping

def measurements_processor(node_to_log_map):
    probes = []

    for node, lines in node_to_log_map.iteritems():
        for line in lines:
            for m in re.finditer("\[TM:(.+?):(.+?)\]", line):
                probe_name = m.group(1)
                probe_value = m.group(2)

                probes.append("[{}] {}: {}\n".format(node, probe_name, probe_value))

    node_to_log_map["probes"] = probes

if __name__ == "__main__":
    lines = sys.stdin.readlines()

    node_to_log_mapping = {"_": []}
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

