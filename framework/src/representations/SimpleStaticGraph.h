//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include <GraphPartition.h>

/* complete graph with 4 vertices */
#define E_N 12
#define V_N 4

extern int E[E_N];
extern int V_OFFSETS[V_N];

class SimpleStaticGraph : public GraphPartition {
public:
	SimpleStaticGraph();

	virtual int getLocalVertexCount() override;

	virtual void forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) override;
	virtual void forEachLocalVertex(std::function<void(LocalVertexId)> f) override;

	virtual bool isLocalVertex(GlobalVertexId id) override;
	virtual NodeId getNodeId() override;
	virtual unsigned long long toNumerical(GlobalVertexId id) override;

private:
	int world_size;
	int world_rank;

	int base_allocation;
	int excess;

	int first;
	int one_after_last;

	int get_node_id_for(int V, int N, int current_rank, int id);
	std::pair <int, int> get_process_range(int rank);
};

#endif //FRAMEWORK_SIMPLESTATICGRAPH_H
