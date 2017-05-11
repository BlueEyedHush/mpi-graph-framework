//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPH_H
#define FRAMEWORK_GRAPH_H

#include <cinttypes>

template <class T> class Iterator {
public:
	virtual bool hasNext() = 0;
	virtual T next() = 0;
	virtual ~Iterator() {};
};

class Graph {
public:
	virtual int getVertexCount() = 0;
	virtual int getEdgeCount() = 0;

	virtual Iterator<int> *getNeighbourIterator(int vertexId) = 0;

	virtual ~Graph() {};
};

void printGraph(Graph *g);

#endif //FRAMEWORK_GRAPH_H
