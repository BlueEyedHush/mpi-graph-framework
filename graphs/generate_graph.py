
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

def output_edge_list_t(nx_graph, file):
    for (start, end) in nx_graph.edges_iter():
        file.write("{}\t{}\n".format(start, end))
        file.write("{}\t{}\n".format(end, start))

def output_graph(filename_prefix, nx_graph, include_test_formats=False):
    bench_formats = [(".adjl", output_adjacency_list), (".elt", output_edge_list_t)]
    testonly_format = [(".vl", output_vertex_list), (".el", output_edge_list)]

    formats = bench_formats if not include_test_formats else bench_formats + testonly_format

    for ext, writer in formats:
        f = open("data/" + filename_prefix + ext, "w")
        writer(nx_graph, f)
        f.close()

def gen_powerlaw(vertices, edges_per_vertex):
    g = nx.powerlaw_cluster_graph(vertices, edges_per_vertex, 0.5, 876)
    fname = "powergraph_{}_{}".format(vertices, len(g.edges()))
    output_graph(fname, g)

if __name__ == '__main__':
    # gen_powerlaw(100000, 10)
    # gen_powerlaw(10000, 10)
    # gen_powerlaw(1000, 10)
    # gen_powerlaw(200000, 10)
    # gen_powerlaw(50000, 10)
    gen_powerlaw(60000, 10)
    gen_powerlaw(70000, 10)
    gen_powerlaw(80000, 10)
    gen_powerlaw(90000, 10)
