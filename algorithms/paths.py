#!/usr/bin/env python

import networkx as nx
from testing import run_tests, G_negative_edges, G_negative_cycle, with_weight_w, G_spt_different_weights, G_disconnected_graph
from heapq import *
from sys import maxint
import math

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

def _find_max_weight(G):
    max_weight = 0
    for u,v in G.edges():
        w = int(G[u][v]["weight"])
        if w < 0:
            raise Exception("Dial doesn't work with negative weights")
        elif w > max_weight:
            max_weight = w
    return max_weight

# casts weights using int(w)
# you can't use bucket_no-1 as initial distance - won't work for vertices which are not reachable
def dial(G, source):
    node_count = len(G.nodes())

    # find max weight
    max_weight = _find_max_weight(G)

    buckets_no = max_weight*node_count
    id_to_initial_dist = lambda id: 0 if id == source else buckets_no-1
    permament_labels_count = 0
    distances = map(id_to_initial_dist, range(0, node_count))
    buckets = [[] for _ in xrange(buckets_no)]

    buckets[0].append(source)
    buckets[-1] = filter(lambda id: id != source, range(0, node_count))

    def bucket_pop():
        for i in range(0, len(buckets)):
            if buckets[i]:
                v = buckets[i][0]
                buckets[i] = buckets[i][1:]
                return v
        raise Exception("all buckets empty - this should never happen!")

    while permament_labels_count < node_count:
        v = bucket_pop()
        for u in G.neighbors(v):
            dist_through_v = distances[v] + int(G[u][v]["weight"])
            u_curr_dist = distances[u]

            if dist_through_v < u_curr_dist:
                buckets[u_curr_dist].remove(u)
                buckets[dist_through_v].append(u)
                distances[u] = dist_through_v

        permament_labels_count += 1

    return list_to_dict(distances)

def _list_of_lists(size):
    return [[] for _ in xrange(size)]

delta = 1.0

# reuse buckets
# handle undirected graphs more efficiently?
def delta_stepping(G, source):
    G = ensure_directed(G)

    node_count = len(G.nodes())
    max_weight = _find_max_weight(G)
    buckets_no = int(math.ceil((max_weight*node_count)/delta))

    heavy_e = _list_of_lists(node_count)
    light_e = _list_of_lists(node_count)

    for u,v in G.edges():
        w = G[u][v]["weight"]
        if w < delta:
            light_e[u].append((u,v))
        else:
            heavy_e[u].append((u,v))

    distances = [float("Inf")] * node_count

    buckets = _list_of_lists(buckets_no)

    def idx(dist):
        return buckets_no-1 if dist == float("Inf") else int(math.floor(dist/delta))

    def relax(v, new_dist):
        curr_dist = distances[v]
        if new_dist < curr_dist:
            buckets[idx(curr_dist)].remove(v)
            buckets[idx(new_dist)].append(v)
            distances[v] = new_dist

    def try_relax(edges):
        for u,v in edges:
            dist_via_u = distances[u] + G[u][v]["weight"]
            relax(v, dist_via_u)

    distances[source] = 0
    buckets[0].append(source)
    buckets[-1] = filter(lambda v: v != source, G.nodes())

    for bucket_id in xrange(buckets_no):
        S = []
        # process light edges
        while buckets[bucket_id]:
            v = buckets[bucket_id][0]
            buckets[bucket_id] = buckets[bucket_id][1:]

            try_relax(light_e[v])

            S.append(v)

        # process heavy edges
        for v in S:
            try_relax(heavy_e[v])

    return list_to_dict(distances)

### testing ###

# output expected from impl: {0: 0, 1: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7, 8: 8, 9: 9}
def sssp_test(impl, G, source):
    pred, expected_dists = nx.bellman_ford(G, source, weight="weight")
    actual = impl(G, source)
    actual_without_unreachable = dict(filter(lambda (k,v): v != float("Inf"), actual.iteritems()))
    return actual_without_unreachable, expected_dists

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
    lambda: sssp_test(dijkstra, with_weight_w(nx.path_graph(10)), 0), # 0
    lambda: sssp_test(dijkstra, with_weight_w(nx.star_graph(10), 2.0), 0), # 1
    lambda: sssp_test(dijkstra, with_weight_w(nx.complete_graph(10)), 0), # 2
    lambda: sssp_test(dijkstra, with_weight_w(nx.circular_ladder_graph(20), 2.0), 0), # 3
    lambda: sssp_negative_cycle_test(bellman_ford, G_negative_cycle, 0), # 4
    lambda: sssp_test(bellman_ford, with_weight_w(nx.path_graph(10)), 0), # 5
    lambda: sssp_test(bellman_ford, with_weight_w(nx.star_graph(10), 2.0), 0), # 6
    lambda: sssp_test(bellman_ford, with_weight_w(nx.complete_graph(10)), 0), # 7
    lambda: sssp_test(bellman_ford, with_weight_w(nx.circular_ladder_graph(20), 2.0), 0), # 8
    lambda: sssp_test(bellman_ford, with_weight_w(nx.circular_ladder_graph(4), 2.0), 0), # 9
    lambda: sssp_test(bellman_ford, with_weight_w(nx.circular_ladder_graph(6), 2.0), 0), # 10
    lambda: sssp_test(bellman_ford, G_negative_edges, 0), # 11
    lambda: sssp_test(dial, with_weight_w(nx.path_graph(10)), 0), # 12
    lambda: sssp_test(dial, with_weight_w(nx.star_graph(10), 2.0), 0), # 13
    lambda: sssp_test(dial, with_weight_w(nx.complete_graph(10)), 0), # 14
    lambda: sssp_test(dial, with_weight_w(nx.circular_ladder_graph(20), 2.0), 0), # 15
    lambda: sssp_test(dial, G_spt_different_weights, 0), # 16
    lambda: sssp_test(delta_stepping, with_weight_w(nx.path_graph(10)), 0), # 17
    lambda: sssp_test(delta_stepping, with_weight_w(nx.star_graph(10), 2.0), 0), # 18
    lambda: sssp_test(delta_stepping, with_weight_w(nx.complete_graph(10)), 0), # 19
    lambda: sssp_test(delta_stepping, with_weight_w(nx.circular_ladder_graph(20), 2.0), 0), # 20
    lambda: sssp_test(delta_stepping, G_spt_different_weights, 0), # 21
    lambda: sssp_test(delta_stepping, G_disconnected_graph, 0), # 22
    lambda: sssp_test(dial, G_disconnected_graph, 0), # 23
    lambda: sssp_test(bellman_ford, G_disconnected_graph, 0), # 24
    lambda: sssp_test(dijkstra, G_disconnected_graph, 0), # 25
]

if __name__ == "__main__":
    run_tests(tests)