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
    visited = []
    start_node = G.nodes()[0]
    vertices = OrdSet()
    vertices.extend([start_node])
    while vertices:
        cv = vertices.fifo()
        visited.append(cv)
        path.append(cv)
        cn = filter(lambda v: v not in visited, G.neighbors(cv))
        print(u"Visiting {}, neighbours {}, vertices {}".format(cv, cn, vertices))
        vertices.extend(cn)
    return path

def vertices_to_edges(vertices):
    edges = []
    for i in range(0, len(vertices) - 1):
        edges.append((vertices[i], vertices[i+1]))
    return edges

def test(actual, expected):
    if(actual == expected):
        return (True, "")
    else:
        return (False, "Actual: {}, Expected: {}".format(actual, expected))

def bfs_test(G):
    start = 0
    actual = vertices_to_edges(bfs(G, start))
    expected = list(nx.dfs_edges(G,start))
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
            print("Test {} succeeded")
        else:
            print("Test {} failed: {}", msg)

if __name__ == "__main__":
    run_tests()