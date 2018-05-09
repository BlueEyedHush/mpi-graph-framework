
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

if __name__ == "__main__":
    lines = sys.stdin.readlines()

    node_to_log_mapping = {"_": []}
    for line in lines:
        splitter(line, node_to_log_mapping)

    node_to_str = {}
    for k in sorted(node_to_log_mapping.iterkeys()):
        node_to_str[k] = "".join(node_to_log_mapping[k])

    # print to separate files
    for k in node_to_str.iterkeys():
        f = open("/tmp/{}_out".format(k), "w")
        f.write(node_to_str[k])
        f.close()

    # print to stdout everything
    for k in sorted(node_to_str.iterkeys()):
        sys.stdout.write("~~~~ {} ~~~~\n".format(k))
        sys.stdout.write(node_to_str[k])
        sys.stdout.write("\n\n")

