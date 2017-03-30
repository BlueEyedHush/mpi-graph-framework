#!/usr/bin/env python

from array import array
import networkx as nx
from testing import run_tests

def _dfs(G, start_id, finish_func):
    visited = [False] * len(G.nodes())

    def _dfs_r(vertex_id):
        # visiting vertex_id
        visited[vertex_id] = True
        # visiting it's neighbours
        for neigh_id in filter(lambda i: not visited[i], G.neighbors(vertex_id)):
            _dfs_r(neigh_id)
        finish_func(vertex_id)

    _dfs_r(start_id)

def digraph_scc_count(G):
    pass

def dfs_test(G):
    postorder_visit = []
    _dfs(G, 0, lambda id: postorder_visit.append(id))
    # all vertices visited (and nothing more)
    return sorted(postorder_visit), sorted(G.nodes())

def scc_test(G):
    return digraph_scc_count(G), nx.number_strongly_connected_components(G)

rnd_graph = nx.fast_gnp_random_graph(10, 50, directed=True)

def disjoint_union(Gs):
    return reduce(nx.disjoint_union, Gs[1:], Gs[0])

tests = [
    lambda: dfs_test(nx.path_graph(10)),
    lambda: scc_test(disjoint_union([rnd_graph, rnd_graph, rnd_graph])),
]

if __name__ == "__main__":
    run_tests(tests)