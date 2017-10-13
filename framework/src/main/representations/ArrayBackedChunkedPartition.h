//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include <GraphPartition.h>
#include <GraphBuilder.h>

struct ABCPGlobalVertexId : public GlobalVertexId {
	ABCPGlobalVertexId(int nodeId, int localId) : nodeId(nodeId), localId(localId) {}

	int nodeId;
	int localId;
};

class ABCPGraphBuilder : public GraphBuilder {
public:
	ABCPGraphBuilder(int partitionCount, int partitionId);
	virtual GraphPartition* buildGraph(std::string path,
	                                   std::vector<OriginalVertexId> verticesToConvert) override;
	virtual std::vector<GlobalVertexId*> getConvertedVertices() override;
	virtual void destroyConvertedVertices() override;
	virtual void destroyGraph(const GraphPartition*) override;
	virtual ~ABCPGraphBuilder() override;
private:
	int P;
	int partitionId;
	std::vector<GlobalVertexId*> convertedVertices;
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
