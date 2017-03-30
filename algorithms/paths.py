#!/usr/bin/env python

import networkx as nx
from testing import run_tests

def dijkstra(G, source):
    pass

# output expected from impl: {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 9}
def sssp_test(impl, G, source):
    expected = nx.single_source_shortest_path_length(G, source)
    actual = impl(G, source)
    return actual, expected

tests = [
    lambda: sssp_test(dijkstra, nx.path_graph(10), 0),
]

if __name__ == "__main__":
    run_tests(tests)