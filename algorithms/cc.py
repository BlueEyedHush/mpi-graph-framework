#!/usr/bin/env python

import networkx as nx
from testing import run_tests

def digraph_scc_count(G):
    return 1

def scc_test(G):
    return digraph_scc_count(G), nx.number_strongly_connected_components(G)


rnd_graph = nx.fast_gnp_random_graph(10, 50, directed=True)

def disjoint_union(Gs):
    return reduce(nx.disjoint_union, Gs[1:], Gs[0])

tests = [
    lambda: scc_test(disjoint_union([rnd_graph, rnd_graph, rnd_graph]))
]

if __name__ == "__main__":
    run_tests(tests)