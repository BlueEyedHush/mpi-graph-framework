//
// Created by blueeyedhush on 26.05.17.
//

#ifndef FRAMEWORK_ADJACENCYLISTHASHPARTITION_H
#define FRAMEWORK_ADJACENCYLISTHASHPARTITION_H

#include <GraphBuilder.h>
#include <GraphPartition.h>

class ALHPGraphBuilder : public GraphBuilder {
public:
	virtual GraphPartition* buildGraph(std::string path, GraphPartition *memory);
};

struct GraphData {
	int world_size;
	int world_rank;

	MPI_Win vertexEdgeWin, adjListWin, offsetTableWin;
	int offsetTableWinSize;
	int adjListWinSize;
	LocalVertexId *vertexEdgeWinMem;
	LocalVertexId *offsetTableWinMem;
	GlobalVertexId *adjListWinMem;
};

class ALHPGraphPartition : public GraphPartition {
public:
	ALHPGraphPartition(GraphData ds);

	virtual int getLocalVertexCount() override;

	virtual void forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) override;
	virtual void forEachLocalVertex(std::function<void(LocalVertexId)> f) override;

	virtual int getMaxLocalVertexCount() override;
	virtual bool isLocalVertex(GlobalVertexId id) override;
	virtual NodeId getNodeId() override ;
	/**
	 *
	 * @return numerical value which is unique and comparable, but there might be gaps
	 */
	virtual unsigned long long toNumerical(GlobalVertexId id) override;

	virtual ~ALHPGraphPartition() override;

private:
	GraphData data;
};


#endif //FRAMEWORK_ADJACENCYLISTHASHPARTITION_H
