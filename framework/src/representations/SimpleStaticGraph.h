//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include "../Graph.h"

/* complete graph with 4 vertices */
#define E_N 12
#define V_N 4

extern int E[E_N];
extern int V_OFFSETS[V_N];

class NeighIt : public Iterator<int> {
private:
	int nextId;
	const int *neighbours;
	int count;

public:
	NeighIt(int v);
	virtual int next() override;
	virtual bool hasNext() override;
	virtual ~NeighIt() override;
};

class SimpleStaticGraph : public Graph {
public:
	virtual int getVertexCount() override;
	virtual int getEdgeCount() override;
	/*
	 * User is responsible for removing iterator
	 */
	virtual Iterator<int> *getNeighbourIterator(int vertexId) override;
};

#endif //FRAMEWORK_SIMPLESTATICGRAPH_H
