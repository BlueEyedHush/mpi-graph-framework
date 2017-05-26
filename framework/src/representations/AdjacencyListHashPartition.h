//
// Created by blueeyedhush on 26.05.17.
//

#ifndef FRAMEWORK_ADJACENCYLISTHASHPARTITION_H
#define FRAMEWORK_ADJACENCYLISTHASHPARTITION_H

#include <GraphBuilder.h>
#include <GraphPartition.h>

class ALHPGraphBuilder : public GraphBuilder {
	virtual GraphPartition* buildGraph(std::string path, GraphPartition *memory);
};

struct GraphData {
	int world_size;
	int world_rank;

	MPI_Win vertexEdgeWin, adjListWin, offsetTableWin;
	LocalVertexId *vertexEdgeWinMem;
	LocalVertexId *offsetTableWinMem;
	GlobalVertexId *adjListWinMem;
};

class ALHPGraphPartition : public GraphPartition {
public:
	ALHPGraphPartition(GraphData ds);

	virtual int getLocalVertexCount();

	virtual void forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f);
	virtual void forEachLocalVertex(std::function<void(LocalVertexId)> f);

	virtual bool isLocalVertex(GlobalVertexId id);
	virtual NodeId getNodeId();
	/**
	 *
	 * @return numerical value which is unique and comparable, but there might be gaps
	 */
	virtual unsigned long long toNumerical(GlobalVertexId id);

	virtual ~ALHPGraphPartition();

private:
	GraphData dataSpaces;
};


#endif //FRAMEWORK_ADJACENCYLISTHASHPARTITION_H
