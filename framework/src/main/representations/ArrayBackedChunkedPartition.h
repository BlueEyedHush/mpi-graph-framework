//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include <GraphPartition.h>

class ArrayBackedChunkedPartition : public GraphPartition {
public:
	ArrayBackedChunkedPartition(int E, int V, int P, int partitionId, int *adjacencyList, int *offsets);

	virtual int getLocalVertexCount() override;

	virtual void forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) override;
	virtual void forEachLocalVertex(std::function<void(LocalVertexId)> f) override;

	virtual bool isLocalVertex(GlobalVertexId id) override;
	virtual NodeId getNodeId() override;
	virtual unsigned long long toNumerical(GlobalVertexId id) override;

private:
	int E;
	int V;
	int P;
	int partitionId;

	int *adjacencyList;
	int *offsets;

	int first;
	int one_after_last;
};

#endif //FRAMEWORK_SIMPLESTATICGRAPH_H
