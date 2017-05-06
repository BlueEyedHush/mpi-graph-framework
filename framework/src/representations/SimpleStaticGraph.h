//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include "../Graph.h"

/* complete graph with 4 vertices */
#define E_N 12
#define V_N 4

extern VERTEX_ID_TYPE E[E_N];
extern SIZE_TYPE V_OFFSETS[V_N];

class NeighIt : public Iterator<VERTEX_ID_TYPE> {
private:
	VERTEX_ID_TYPE nextId;
	const VERTEX_ID_TYPE *neighbours;
	SIZE_TYPE count;

public:
	NeighIt(VERTEX_ID_TYPE v);
	virtual VERTEX_ID_TYPE next() override;
	virtual bool hasNext() override;
	virtual ~NeighIt() override;
};

class SimpleStaticGraph : public Graph {
public:
	virtual SIZE_TYPE getVertexCount() override;
	virtual SIZE_TYPE getEdgeCount() override;
	/*
	 * User is responsible for removing iterator
	 */
	virtual Iterator<VERTEX_ID_TYPE> *getNeighbourIterator(VERTEX_ID_TYPE vertexId) override;
};

#endif //FRAMEWORK_SIMPLESTATICGRAPH_H
