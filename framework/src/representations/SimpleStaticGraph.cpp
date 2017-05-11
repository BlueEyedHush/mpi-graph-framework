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

NeighIt::NeighIt(int v) : nextId(0) {
	neighbours = &E[V_OFFSETS[v]];
	count = nextId < (v != V_N - 1) ? (V_OFFSETS[v+1] - V_OFFSETS[v]) : (E_N - V_OFFSETS[V_N-1]);
}

int NeighIt::next() {
	int el = neighbours[nextId];
	nextId++;
	return el;
}

bool NeighIt::hasNext() {
	return nextId < count;
}

NeighIt::~NeighIt() {}

int SimpleStaticGraph::getVertexCount() {
	return V_N;
}

int SimpleStaticGraph::getEdgeCount() {
	return E_N;
}

Iterator<int> *SimpleStaticGraph::getNeighbourIterator(int vertexId) {
	return new NeighIt(vertexId);
}
