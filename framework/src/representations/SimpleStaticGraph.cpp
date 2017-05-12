//
// Created by blueeyedhush on 06.05.17.
//

#include "SimpleStaticGraph.h"

int E[E_N] = {
	1, 2, 3,
	0, 2, 3,
	0, 1, 3,
	0, 1, 2
};

int V_OFFSETS[V_N] = {
	0,
	3,
	6,
	9
};

int SimpleStaticGraph::getVertexCount() {
	return V_N;
}

int SimpleStaticGraph::getEdgeCount() {
	return E_N;
}

void SimpleStaticGraph::forEachNeighbour(int v, std::function<void(int)> f) {
	int *neighbours = &E[V_OFFSETS[v]];
	int count = (v != V_N - 1) ? (V_OFFSETS[v+1] - V_OFFSETS[v]) : (E_N - V_OFFSETS[V_N-1]);

	for(int i = 0; i < count; i++) {
		f(neighbours[i]);
	}
}
