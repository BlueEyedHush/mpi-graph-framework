#!/usr/bin/env python

import networkx as nx

def bfs(G, start):
    # get vertices
    # get neighbours from vertex

    return list(nx.dfs_edges(G,0))

def test():
    G = nx.complete_graph(10)
    start = 0
    actual = bfs(G, start)
    expected = list(nx.dfs_edges(G,start))
    if(actual == expected):
        print("success")
    else:
        print("failure")

if __name__ == "__main__":
    test()