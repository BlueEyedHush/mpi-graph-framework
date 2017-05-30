
import os
import networkx as nx

dir_path = os.path.dirname(os.path.realpath(__file__))

def build_path(gname):
    return os.path.join(dir_path, "graphs", gname + ".adjl")

def save_adj_list(nx_graph, name):
    with open(build_path(name), "wb") as f:
        f.write("{}\n".format(nx_graph.number_of_nodes()))
        f.write("{}\n".format(nx_graph.number_of_edges()))

        for v in nx_graph.nodes():
            ns = list(nx_graph.neighbors(v))
            line = "{} {}\n".format(v, " ".join(map(lambda v: str(v), ns)))
            f.write(line)

if __name__ == '__main__':
    g = nx.complete_graph(10)
    save_adj_list(g, "test")