//
// Created by blueeyedhush on 06.05.17.
//

#include "SimpleStaticGraph.h"

VERTEX_ID_TYPE E[E_N] = {
	1, 2, 3,
	0, 2, 3,
	0, 1, 3,
	0, 1, 2
};

VERTEX_ID_TYPE V_OFFSETS[V_N] = {
	0,
	3,
	6,
	9
};

NeighIt::NeighIt(VERTEX_ID_TYPE v) : nextId(0) {
	neighbours = &E[V_OFFSETS[v]];
	count = nextId < (v != V_N - 1) ? (V_OFFSETS[v+1] - V_OFFSETS[v]) : (E_N - V_OFFSETS[V_N-1]);
}

VERTEX_ID_TYPE NeighIt::next() {
	VERTEX_ID_TYPE el = neighbours[nextId];
	nextId++;
	return el;
}

bool NeighIt::hasNext() {
	return nextId < count;
}

NeighIt::~NeighIt() {}

SIZE_TYPE SimpleStaticGraph::getVertexCount() {
	return V_N;
}

SIZE_TYPE SimpleStaticGraph::getEdgeCount() {
	return E_N;
}

Iterator<VERTEX_ID_TYPE> *SimpleStaticGraph::getNeighbourIterator(VERTEX_ID_TYPE vertexId) {
	return new NeighIt(vertexId);
}
