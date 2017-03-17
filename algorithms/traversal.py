#!/usr/bin/env python

import networkx as nx
from collections import OrderedDict

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
    # same vertices are added multiple times to the vertices list
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

def test(actual, expected):
    if(actual == expected):
        return (True, "")
    else:
        return (False, "Actual: {}, Expected: {}".format(actual, expected))

def bfs_test(G):
    start = 0
    actual = bfs(G, start)
    expected = list(nx.bfs_edges(G,start))
    return actual, expected

tests = [
    lambda: bfs_test(nx.complete_graph(10)),
    lambda: bfs_test(nx.path_graph(10)),
    lambda: bfs_test(nx.star_graph(10))
]

def run_tests():
    for id, t in zip(range(0, len(tests)), tests):
        actual, expected = t()
        success, msg = test(actual, expected)
        if success:
            print("Test {} succeeded".format(id))
        else:
            print("Test {} failed: {}".format(id, msg))

if __name__ == "__main__":
    run_tests()