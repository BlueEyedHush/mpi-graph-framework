//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include <Graph.h>

/* complete graph with 4 vertices */
#define E_N 12
#define V_N 4

extern int E[E_N];
extern int V_OFFSETS[V_N];

class SimpleStaticGraph : public Graph {
public:
	SimpleStaticGraph();

	virtual int getVertexCount() override;
	virtual int getLocalVertexCount() override;
	virtual int getEdgeCount() override;

	virtual void forEachNeighbour(int vertexId, std::function<void(int)> f) override;

	virtual bool isLocalVertex(int id) override;
	virtual void forEachLocalVertex(std::function<void(int)> f) override;
	virtual int getNodeResponsibleForVertex(int id) override;

private:
	int world_size;
	int world_rank;

	int base_allocation;
	int excess;

	int first;
	int one_after_last;

	std::pair <int, int> get_process_range(int rank);
};

#endif //FRAMEWORK_SIMPLESTATICGRAPH_H
