#!/usr/bin/env python

import networkx as nx
from collections import OrderedDict
from testing import run_tests

class OrdSet:
    def __init__(self):
        self.storage = OrderedDict()

    def extend(self, element_list):
        self.storage.update(map(lambda el: (el, True), element_list))

    def fifo(self):
        el, dummy = self.storage.popitem(last=False)
        return el

    def lifo(self):
        el, dummy = self.storage.popitem(last=True)
        return el

    def __len__(self):
        return len(self.storage)

    def __str__(self):
        return str(self.to_list())

    def to_list(self):
        return self.storage.keys()

def bfs(G, start):
    # this could be simplified if our OrdSet considered elements with only
    # first elements different identical. Since it doesn't, we need separate structure
    # to track visited vertices
    path = []
    start_node = G.nodes()[0]
    visited = {start_node}
    edges = OrdSet()
    edges.extend([(start_node, start_node)])
    while edges:
        # we are looking at neighbours of the edge's end, first is just for recording purposes
        ce = edges.fifo()
        s, e = ce
        # now we perform visit of e
        path.append(ce)
        neighbour_vertices = filter(lambda v: v not in visited, G.neighbors(e))
        visited.update(neighbour_vertices)
        neighbour_edges = map(lambda nv: (e, nv), neighbour_vertices)
        print(u"Visiting {}, neighbours {}, vertices {}".format(s, neighbour_vertices, edges))
        edges.extend(neighbour_edges)
    return path[1:]

def dfs(G, start):
    # this could be simplified if our OrdSet considered elements with only
    # first elements different identical. Since it doesn't, we need separate structure
    # to track visited vertices
    path = []
    start_node = G.nodes()[0]
    visited = {start_node}
    edges = OrdSet()
    edges.extend([(start_node, start_node)])
    while edges:
        # we are looking at neighbours of the edge's end, first is just for recording purposes
        ce = edges.lifo()
        s, e = ce
        # now we perform visit of e
        path.append(ce)
        neighbour_vertices = filter(lambda v: v not in visited, G.neighbors(e))
        visited.update(neighbour_vertices)
        neighbour_edges = map(lambda nv: (e, nv), neighbour_vertices)
        print(u"Visiting {}, neighbours {}, vertices {}".format(s, neighbour_vertices, edges))
        edges.extend(neighbour_edges)
    return path[1:]

### testing ###

def bfs_test(G):
    start = 0
    return bfs(G, start), list(nx.bfs_edges(G,start))

def dfs_test(G):
    start = 0
    return dfs(G, start), list(nx.dfs_edges(G,start))

tests = [
    lambda: bfs_test(nx.complete_graph(10)),
    lambda: bfs_test(nx.path_graph(10)),
    lambda: bfs_test(nx.star_graph(10)),
    lambda: dfs_test(nx.complete_graph(10)),
    lambda: dfs_test(nx.path_graph(10)),
    lambda: dfs_test(nx.star_graph(10)),
]

if __name__ == "__main__":
    run_tests(tests)