
import sys
import networkx as nx

def output_adj_list(nx_graph):
    edge_count = 0
    neighbours_str = ""
    for v in nx_graph.nodes():
        ns = list(nx_graph.neighbors(v))
        line = "{} {}\n".format(v, " ".join(map(lambda v: str(v), ns)))
        neighbours_str += line
        edge_count += len(ns)

    sys.stdout.write("{}\n".format(nx_graph.number_of_nodes()))
    sys.stdout.write("{}\n".format(edge_count))
    sys.stdout.write(neighbours_str)

if __name__ == '__main__':
    g = nx.complete_graph(50)
    output_adj_list(g)