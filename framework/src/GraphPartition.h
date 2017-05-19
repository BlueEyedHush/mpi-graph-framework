//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPH_H
#define FRAMEWORK_GRAPH_H

#include <cinttypes>
#include <functional>

typedef int NodeId;
typedef int LocalVertexId;

struct GlobalVertexId {
	GlobalVertexId() {GlobalVertexId(-1, -1);}
	GlobalVertexId(NodeId nId, LocalVertexId lvId) : nodeId(nId), localId(lvId) {}

	NodeId nodeId;
	LocalVertexId localId;
};

class GraphPartition {
public:
	virtual int getLocalVertexCount() = 0;

	virtual void forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) = 0;
	virtual void forEachLocalVertex(std::function<void(LocalVertexId)> f) = 0;

	virtual bool isLocalVertex(GlobalVertexId id) = 0;
	virtual NodeId getNodeId() = 0;
	/**
	 *
	 * @return numerical value which is unique and comparable, but there might be gaps
	 */
	virtual unsigned long long toNumerical(GlobalVertexId id) = 0;

	virtual ~GraphPartition() {};
};

#endif //FRAMEWORK_GRAPH_H
