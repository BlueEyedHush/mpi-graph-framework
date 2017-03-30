#!/usr/bin/env python

import networkx as nx
from testing import run_tests
from heapq import *

negative_cycle = Exception("negative_cycle")

def is_negative_cycle_ex(e):
    return e.args[0] == negative_cycle.args[0]

def heapqdel(q, vertex_id):
    new_q = filter(lambda (dist, id): id != vertex_id, q)
    heapify(new_q)
    return new_q

def list_to_dict(list):
    dict = {}
    for i in range(0, len(list)):
        dict[i] = list[i]
    return dict

def ensure_directed(G):
    return G.to_directed() if not nx.is_directed(G) else G

def dijkstra(G, source):
    id_to_initial_dist = lambda id: 0.0 if id == source else float('Inf')

    node_count = len(G.nodes())
    distances = map(id_to_initial_dist, G.nodes())
    predecessors = [-1] * node_count

    q = map(lambda id: (id_to_initial_dist(id), id), G.nodes())
    heapify(q)

    while len(q) > 0:
        v_dist, v = heappop(q)
        for neigh_id in G.neighbors(v):
            dist_through_v = v_dist + G[v][neigh_id]['weight']
            if dist_through_v < distances[neigh_id]:
                q = heapqdel(q, neigh_id)

                distances[neigh_id] = dist_through_v
                predecessors[neigh_id] = v

                heappush(q, (dist_through_v, neigh_id))

    return list_to_dict(distances)

def bellman_ford(G, source):
    # if we are working with undirected graph we need edges back & forth
    G = ensure_directed(G)

    node_count = len(G.nodes())

    id_to_initial_dist = lambda id: 0.0 if id == source else float('Inf')
    distances = map(id_to_initial_dist, range(0, node_count))

    def try_shorten():
        shortened_no = 0

        for u, v in G.edges():
            w = G[u][v]["weight"]

            dist_through_u = distances[u] + w
            if dist_through_u < distances[v]:
                distances[v] = dist_through_u
                shortened_no += 1

        return shortened_no

    for i in range(0, node_count-1):
        try_shorten()

    # check for negative cycle
    if try_shorten() > 0:
        raise negative_cycle

    return list_to_dict(distances)

### testing ###
ww = lambda u,v,w: (u, v, {"weight": float(w)})

G_negative_edges = \
    nx.DiGraph([
        ww(0,1,1), ww(1,2,2), ww(2,3,3), ww(3,4,0),
        ww(1,5,1), ww(5,3,1),
        ww(1,6,10), ww(6,7,-11), ww(7,8,-1), ww(8,3,1),
    ])

G_negative_cycle = \
    nx.DiGraph([
        ww(0,1,2), ww(2,3,4),
        ww(1,2,-1), ww(2,1,-1), # negative cycle
        ww(1,4,1), ww(4,5,1), ww(5,2,1), # alternative path without negative cycle
    ])

def with_weight_w(G, w = 1.0):
    for u, v in G.edges():
        G[u][v]['weight'] = w
    return G

# output expected from impl: {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 9}
def sssp_test(impl, G, source):
    pred, expected_dists = nx.bellman_ford(G, source, weight="weight")
    actual = impl(G, source)
    return actual, expected_dists

def sssp_negative_cycle_test(impl, G, source):
    thrown = "{} thrown".format(negative_cycle)
    not_thrown = "{} not thrown".format(negative_cycle)
    try:
        impl(G, source)
        return not_thrown, thrown
    except Exception as e:
        if is_negative_cycle_ex(e):
            return thrown, thrown
        else:
            raise

tests = [
    # Dijkstra
    lambda: sssp_test(dijkstra, with_weight_w(nx.path_graph(10)), 0), # 0
    lambda: sssp_test(dijkstra, with_weight_w(nx.star_graph(10), 2.0), 0), # 1
    lambda: sssp_test(dijkstra, with_weight_w(nx.complete_graph(10)), 0), # 2
    lambda: sssp_test(dijkstra, with_weight_w(nx.circular_ladder_graph(20), 2.0), 0), # 3
    # Bellman-Ford
    lambda: sssp_negative_cycle_test(bellman_ford, G_negative_cycle, 0), # 4
    lambda: sssp_test(bellman_ford, with_weight_w(nx.path_graph(10)), 0), # 5
    lambda: sssp_test(bellman_ford, with_weight_w(nx.star_graph(10), 2.0), 0), # 6
    lambda: sssp_test(bellman_ford, with_weight_w(nx.complete_graph(10)), 0), # 7
    lambda: sssp_test(bellman_ford, with_weight_w(nx.circular_ladder_graph(20), 2.0), 0), # 8
    lambda: sssp_test(bellman_ford, with_weight_w(nx.circular_ladder_graph(4), 2.0), 0), # 9
    lambda: sssp_test(bellman_ford, with_weight_w(nx.circular_ladder_graph(6), 2.0), 0), # 10
    lambda: sssp_test(bellman_ford, G_negative_edges, 0), # 11
]

if __name__ == "__main__":
    run_tests(tests)