
import sys
import networkx as nx

def output_adjacency_list(nx_graph, file):
    edge_count = 0
    neighbours_str = ""
    for v in nx_graph.nodes():
        ns = list(nx_graph.neighbors(v))
        line = "{} {}\n".format(v, " ".join(map(lambda v: str(v), ns)))
        neighbours_str += line
        edge_count += len(ns)

    file.write("{}\n".format(nx_graph.number_of_nodes()))
    file.write("{}\n".format(edge_count))
    file.write(neighbours_str)

def output_vertex_list(nx_graph, file):
    vertex_str = ",".join(map(lambda i: str(i), nx_graph.nodes()))
    file.write(vertex_str)

def output_edge_list(nx_graph, file):
    for (start, end) in nx_graph.edges_iter():
        file.write("{},{}\n".format(start, end))
        file.write("{},{}\n".format(end, start))

def output_graph(filename_prefix, nx_graph):

    for ext, writer in [(".adjl", output_adjacency_list), (".vl", output_vertex_list), (".el", output_edge_list)]:
        f = open(filename_prefix + ext, "w")
        writer(nx_graph, f)
        f.close()


if __name__ == '__main__':
    g = nx.powerlaw_cluster_graph(25, 2, 0.5, 876)

    output_graph(sys.argv[1] if len(sys.argv) > 1 else "graph", g)