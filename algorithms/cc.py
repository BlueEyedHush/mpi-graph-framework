#!/usr/bin/env python

from array import array
import networkx as nx
from testing import run_tests

def _dfs(G, start_id, finish_func, node_filter = lambda id: True):
    visited = [False] * len(G.nodes())

    def _dfs_r(vertex_id):
        if not visited[vertex_id]:
            # visiting vertex_id
            visited[vertex_id] = True
            # visiting it's neighbours, check for visited is not really needed
            for neigh_id in filter(lambda i: not visited[i] and node_filter(vertex_id), G.neighbors(vertex_id)):
                _dfs_r(neigh_id)
            finish_func(vertex_id)

    _dfs_r(start_id)

# merge visited & component
def digraph_scc_count(G):
    node_count = len(G.nodes())

    component_ids = [-1] * node_count
    next_id = 0

    # first DFS traversal, build prospective components
    vertices_postorder = []

    for i in G.nodes():
        if(component_ids[i] == -1):
            def postfinish(id):
                vertices_postorder.append(id)
                component_ids[id] = next_id
            _dfs(G, i, postfinish)
            next_id += 1

    # invert graph
    Ginv = G.reverse()

    # final DFS traversal
    vertices_postorder.reverse()
    second_iteration_traversal_order = vertices_postorder

    final_components_ids = [-1] * node_count
    next_id = 0

    for start_vertex_id in second_iteration_traversal_order:
        if final_components_ids[start_vertex_id] == -1:
            def postfinish(id):
                final_components_ids[id] = next_id

            _dfs(Ginv, start_vertex_id, postfinish, lambda id: component_ids[id] == component_ids[start_vertex_id])
            next_id += 1

    return next_id


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