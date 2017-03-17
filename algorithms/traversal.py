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

def test1():
    G = nx.complete_graph(10)
    start = 0
    actual = vertices_to_edges(bfs(G, start))
    expected = list(nx.dfs_edges(G,start))
    return actual == expected

def test2():
    G = nx.path_graph(10)
    start = 0
    actual = vertices_to_edges(bfs(G, start))
    expected = list(nx.dfs_edges(G,start))
    return actual == expected

if __name__ == "__main__":
    if test1():
        print("test1 success")
    else:
        print("test1 failure")

    if test2():
        print("test2 success")
    else:
        print("test2 failure")