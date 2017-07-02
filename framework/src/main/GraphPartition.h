//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPH_H
#define FRAMEWORK_GRAPH_H

#include <cinttypes>
#include <functional>

typedef int NodeId;
typedef int LocalVertexId;
#define NODE_ID_MPI_TYPE MPI_INT
#define LOCAL_VERTEX_ID_MPI_TYPE MPI_INT

struct GlobalVertexId {
	GlobalVertexId() {GlobalVertexId(-1, -1);}
	GlobalVertexId(NodeId nId, LocalVertexId lvId) : nodeId(nId), localId(lvId) {}

	NodeId nodeId;
	LocalVertexId localId;
};

class GraphPartition {
public:
	virtual void forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) = 0;
	virtual void forEachLocalVertex(std::function<void(LocalVertexId)> f) = 0;

	virtual int getMaxLocalVertexCount() = 0;
	virtual int getLocalVertexCount() = 0;
	virtual bool isLocalVertex(GlobalVertexId id) = 0;
	virtual NodeId getNodeId() = 0;
	virtual unsigned long long toNumerical(GlobalVertexId id) = 0;

	virtual ~GraphPartition() {};
};

#endif //FRAMEWORK_GRAPH_H
