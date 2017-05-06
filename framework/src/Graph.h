//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPH_H
#define FRAMEWORK_GRAPH_H

#include <cinttypes>

#define VERTEX_ID_TYPE uint16_t
#define SIZE_TYPE uint16_t

template <class T> class Iterator {
public:
	virtual bool hasNext() = 0;
	virtual T next() = 0;
	virtual ~Iterator() {};
};

class Graph {
public:
	virtual SIZE_TYPE getVertexCount() = 0;
	virtual SIZE_TYPE getEdgeCount() = 0;

	virtual Iterator<VERTEX_ID_TYPE> *getNeighbourIterator(VERTEX_ID_TYPE vertexId) = 0;

	virtual ~Graph() {};
};

void printGraph(Graph *g);

#endif //FRAMEWORK_GRAPH_H
