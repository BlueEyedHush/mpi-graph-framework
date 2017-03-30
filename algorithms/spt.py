#!/usr/bin/env python

import networkx as nx
from networkx.utils.union_find import UnionFind
from testing import run_tests, with_weight_w

def kruscal(G):
    uf = UnionFind()

    # create singleton sets for every vertex
    # if not is not yet in UnionFind, it'll be added by [] operator
    for u,v in G.edges():
        u_root = uf[u]
        v_root = uf[v]
        if u_root != v_root:
            uf.union(u_root, v_root)

            # terminate if all vertices in uf
            vlist = list(uf)
            if len(vlist) == len(G.nodes()):
                return vlist

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
    es_test_result = edges_without_data(spt_actual) <= edges_without_data(G)

    result_str_repr = lambda vp, ce, es: vertices_present_msg[vp] + "_" + cost_equal_msg[ce] + "_" + edges_subset_msg[es]

    return result_str_repr(vp_test_result, ce_test_result, es_test_result), result_str_repr(True, True, True)

tests = [
    lambda: spt_func_test(nx.minimum_spanning_tree, with_weight_w(nx.complete_graph(10), 2.0)),
    lambda: spt_func_test(kruscal, with_weight_w(nx.complete_graph(10), 2.0)),
]

if __name__ == "__main__":
    run_tests(tests)