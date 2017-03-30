#!/usr/bin/env python

import networkx as nx
from networkx.utils.union_find import UnionFind
from testing import run_tests, with_weight_w, G_spt_different_weights

def kruscal(G):
    node_count = len(G.nodes())
    uf = UnionFind()

    # we need tree as result of this algorithm, not set of vertices
    # so edges are kept in union-find structures for convenience
    # this means that stop condition is uf reaching size of 2*V-1
    # /we need V-1 edges to connect V vertices assuming no cycles/
    def uflist_to_graph(uf_list):
        edges = filter(lambda x: isinstance(x, tuple), uf_list)
        spt = nx.Graph(edges)
        for u,v in edges:
            spt[u][v]["weight"] = G[u][v]["weight"]
        return spt

    sorted_edges = sorted(G.edges(), key = lambda (u,v): G[u][v]["weight"])

    # create singleton sets for every vertex
    # if not is not yet in UnionFind, it'll be added by [] operator
    for u,v in sorted_edges:
        u_root = uf[u]
        v_root = uf[v]
        if u_root != v_root:
            uf.union(u_root, v_root)
            # add edge to union-find
            uf.union(u_root, uf[(u,v)])

            # terminate if all vertices in uf
            uflist = list(uf)
            if len(uflist) == 2*node_count-1:
                return uflist_to_graph(uflist)

    raise Exception("Something went wrong, algorithm should never end up here")

### tests ###

def spt_func_test(impl, G):
    spt_actual = impl(G)

    # all vertices in SPT
    vertices_present_msg = {True: "all_present", False: "vertex_missing"}
    vp_test_result = (len(spt_actual.nodes()) == len(G.nodes())) and (set(spt_actual.nodes()) == set(G.nodes()))

    # summary cost equal to one returned by NetworkX
    cost_equal_msg = {True: "cost_equal", False: "cost_different"}
    calc_cost = lambda G: reduce(lambda a,b: a+b, map(lambda (u, v): G[u][v]["weight"], G.edges()))
    ce_test_result = calc_cost(spt_actual) == calc_cost(nx.minimum_spanning_tree(G))

    # edges in SPT should be subset of edges in G
    edges_subset_msg = {True: "edge_subset", False: "additional_edges"}
    edges_without_data = lambda G: map(lambda t: t[0:2], G.edges())
    # this is needed because for undirected graphs NetworkX has edges only in one direction
    edges_without_data_G = edges_without_data(G)
    edges_both_dirs_G = edges_without_data_G + map(lambda t: tuple(reversed(t)), edges_without_data_G)
    es_test_result = set(edges_without_data(spt_actual)) <= set(edges_both_dirs_G)

    result_str_repr = lambda vp, ce, es: vertices_present_msg[vp] + "_" + cost_equal_msg[ce] + "_" + edges_subset_msg[es]

    return result_str_repr(vp_test_result, ce_test_result, es_test_result), result_str_repr(True, True, True)

tests = [
    lambda: spt_func_test(kruscal, with_weight_w(nx.complete_graph(10), 2.0)),
    lambda: spt_func_test(kruscal, G_spt_different_weights),
]

if __name__ == "__main__":
    run_tests(tests)