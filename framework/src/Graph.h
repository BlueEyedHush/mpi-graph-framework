//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPH_H
#define FRAMEWORK_GRAPH_H

#include <cinttypes>
#include <functional>

class Graph {
public:
	virtual int getVertexCount() = 0;
	virtual int getLocalVertexCount() = 0;
	virtual int getEdgeCount() = 0;

	virtual void forEachNeighbour(int vertexId, std::function<void(int)> f) = 0;

	virtual bool isLocalVertex(int id) = 0;
	virtual void forEachLocalVertex(std::function<void(int)> f) = 0;
	virtual int getNodeResponsibleForVertex(int id) = 0;

	virtual ~Graph() {};
};

void printGraph(Graph *g);

#endif //FRAMEWORK_GRAPH_H
