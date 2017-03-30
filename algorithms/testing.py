
import networkx as nx

def _test(actual, expected):
    if(actual == expected):
        return (True, "")
    else:
        return (False, "Actual: {}, Expected: {}".format(actual, expected))

def run_tests(test_list):
    for id, t in zip(range(0, len(test_list)), test_list):
        actual, expected = t()
        success, msg = _test(actual, expected)
        if success:
            print("Test {} succeeded".format(id))
        else:
            print("Test {} failed: {}".format(id, msg))

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

G_spt_different_weights = \
    nx.Graph([
        ww(0,1,10), ww(1,2,10), ww(2,3,10),
        ww(4,0,1), ww(4,1,2), ww(4,2,3), ww(4,3,4),
        ww(5,0,4), ww(5,1,3), ww(5,2,2), ww(5,3,1),
    ])

def with_weight_w(G, w = 1.0):
    for u, v in G.edges():
        G[u][v]['weight'] = w
    return G

