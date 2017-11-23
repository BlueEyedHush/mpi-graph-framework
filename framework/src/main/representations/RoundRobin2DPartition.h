//
// Created by blueeyedhush on 23.11.17.
//

#ifndef FRAMEWORK_ROUNDROBIN2DPARTITION_H
#define FRAMEWORK_ROUNDROBIN2DPARTITION_H

#include <string>
#include <vector>
#include <GraphPartitionHandle.h>
#include <GraphPartition.h>
#include "shared.h"

template <typename TLocalId, typename TNumId>
class RoundRobin2DPartition : public GraphPartition<ALHPGlobalVertexId<TLocalId>, TLocalId, TNumId> {
	using P = GraphPartition<ALHPGlobalVertexId<TLocalId>, TLocalId, TNumId>;
	IMPORT_ALIASES(P)

public:
	MPI_Datatype getGlobalVertexIdDatatype() {

	}
	
	LocalId toLocalId(const GlobalId, VERTEX_TYPE* vtype = nullptr) {

	}

	NodeId toMasterNodeId(const GlobalId) {

	}

	GlobalId toGlobalId(const LocalId) {

	}

	NumericId toNumeric(const GlobalId) {

	}

	NumericId toNumeric(const LocalId) {

	}

	std::string idToString(const GlobalId) {

	}

	std::string idToString(const LocalId) {

	}

	bool isSame(const GlobalId, const GlobalId) {

	}

	bool isValid(const GlobalId) {

	}


	void foreachMasterVertex(std::function<ITER_PROGRESS (const LocalId)>) {

	}

	size_t masterVerticesCount() {

	}

	size_t masterVerticesMaxCount() {

	}

	
	void foreachCoOwner(LocalId, bool returnSelf, std::function<ITER_PROGRESS (const NodeId)>) {

	}
	
	void foreachNeighbouringVertex(LocalId, std::function<ITER_PROGRESS (const GlobalId)>) {

	}
};

template <typename TLocalId, typename TNumId>
class RR2DHandle : public GraphPartitionHandle<RoundRobin2DPartition<TLocalId, TNumId>> {
	using G = RoundRobin2DPartition<TLocalId, TNumId>;
	using P = GraphPartitionHandle<G>;
	IMPORT_ALIASES(G)

public:
	RR2DHandle(std::string path, std::vector<OriginalVertexId> verticesToConv)
			: GraphPartitionHandle(verticesToConv, destroyGraph)
	{

	}

protected:
	virtual std::pair<RoundRobin2DPartition*, std::vector<GlobalId>>
	buildGraph(std::vector<OriginalVertexId> verticesToConvert) override {

	};

private:
	static void destroyGraph(G* g) {
		// don't forget to free type!
	}
};

#endif //FRAMEWORK_ROUNDROBIN2DPARTITION_H
