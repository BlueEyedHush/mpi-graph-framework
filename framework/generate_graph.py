
import os
import networkx as nx

dir_path = os.path.dirname(os.path.realpath(__file__))

def build_path(gname):
    return os.path.join(dir_path, "resources/test/", gname + ".adjl")

def save_adj_list(nx_graph, name):
    with open(build_path(name), "wbc") as f:

        edge_count = 0
        neighbours_str = ""
        for v in nx_graph.nodes():
            ns = list(nx_graph.neighbors(v))
            line = "{} {}\n".format(v, " ".join(map(lambda v: str(v), ns)))
            neighbours_str += line
            edge_count += len(ns)

        f.write("{}\n".format(nx_graph.number_of_nodes()))
        f.write("{}\n".format(edge_count))
        f.write(neighbours_str)

if __name__ == '__main__':
    g = nx.complete_graph(50)
    save_adj_list(g, "complete50")