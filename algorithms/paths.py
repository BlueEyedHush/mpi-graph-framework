#!/usr/bin/env python

import networkx as nx
from testing import run_tests
from heapq import *
from sys import maxint

def heapqdel(q, vertex_id):
    new_q = filter(lambda (dist, id): id != vertex_id, q)
    heapify(new_q)
    return new_q

def list_to_dict(list):
    dict = {}
    for i in range(0, len(list)):
        dict[i] = list[i]
    return dict

def dijkstra(G, source):
    node_count = len(G.nodes())
    distances = map(lambda id: 0 if id == source else maxint, G.nodes())
    predecessors = [-1] * node_count

    q = map(lambda id: (0 if id == source else maxint, id), G.nodes())
    heapify(q)

    while len(q) > 0:
        v_dist, v = heappop(q)
        for neigh_id in G.neighbors(v):
            dist_through_v = v_dist + 1
            if dist_through_v < distances[neigh_id]:
                q = heapqdel(q, neigh_id)

                distances[neigh_id] = dist_through_v
                predecessors[neigh_id] = v

                heappush(q, (dist_through_v, neigh_id))

    return list_to_dict(distances)

# output expected from impl: {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 9}
def sssp_test(impl, G, source):
    expected = nx.single_source_shortest_path_length(G, source)
    actual = impl(G, source)
    return actual, expected

tests = [
    lambda: sssp_test(dijkstra, nx.path_graph(10), 0),
    lambda: sssp_test(dijkstra, nx.star_graph(10), 0),
    lambda: sssp_test(dijkstra, nx.complete_graph(10), 0),
    lambda: sssp_test(dijkstra, nx.circular_ladder_graph(20), 0),
]

if __name__ == "__main__":
    run_tests(tests)