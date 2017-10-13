//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include <GraphPartition.h>
#include <GraphBuilder.h>

class ABCPGraphBuilder : public GraphBuilder {
public:
	ABCPGraphBuilder(int partitionCount, int partitionId);
	virtual GraphPartition* buildGraph(std::string path);
	virtual void destroyGraph(const GraphPartition*);
private:
	int P;
	int partitionId;
};

class ArrayBackedChunkedPartition : public GraphPartition {
public:
	ArrayBackedChunkedPartition(int E, int V, int P, int partitionId, int *adjacencyList, int *offsets);

	virtual void forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) override;
	virtual void forEachLocalVertex(std::function<void(LocalVertexId)> f) override;

	virtual int getMaxLocalVertexCount() override ;
	virtual int getLocalVertexCount() override;
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
