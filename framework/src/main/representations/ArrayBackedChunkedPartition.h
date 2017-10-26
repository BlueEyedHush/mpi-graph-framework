//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include <algorithm>
#include <climits>
#include <sstream>
#include <glog/logging.h>
#include <boost/pool/object_pool.hpp>
#include <GraphPartition.h>
#include <GraphPartitionHandle.h>
#include <utils/IndexPartitioner.h>
#include <utils/AdjacencyListReader.h>
#include <utils/MpiTypemap.h>

template <typename T>
struct ABCPGlobalVertexId {
	ABCPGlobalVertexId() : nodeId(-1), localId(0) {}
	ABCPGlobalVertexId(NodeId nodeId, T localId) : nodeId(nodeId), localId(localId) {}

	NodeId nodeId;
	T localId;

	static MPI_Datatype mpiDatatype() {
		MPI_Datatype d;

		int blocklengths[] = {1, 1};
		MPI_Aint displacements[] = {offsetof(ABCPGlobalVertexId, nodeId), offsetof(ABCPGlobalVertexId, localId)};
		MPI_Datatype tMpiDatatype = datatypeMap.at(typeid(T));
		MPI_Datatype building_types[] = {NODE_ID_MPI_TYPE, tMpiDatatype};
		MPI_Type_create_struct(2, blocklengths, displacements, building_types, &d);

		return d;
	}

	/* used for testing, interface is GraphPartition::isSame */
	bool operator==(const ABCPGlobalVertexId& o) const {
		return nodeId == o.nodeId && localId == o.localId;
	}

	bool operator!=(const ABCPGlobalVertexId& o) const {
		return !operator==(o);
	}
};

/*
 * For this representation, toNumeric always returns original ID (as loaded from file).
 */
template <typename TLocalId, typename TNumId>
class ArrayBackedChunkedPartition : public GraphPartition<ABCPGlobalVertexId<TLocalId>, TLocalId, TNumId> {
private:
	using P = GraphPartition<ABCPGlobalVertexId<TLocalId>, TLocalId, TNumId>;
	IMPORT_ALIASES(P)

public:
	ArrayBackedChunkedPartition(std::vector<GlobalId> *adjList,
	                            size_t vertexCount,
	                            size_t vertexMaxCount,
	                            NodeId nodeId,
	                            size_t partitionOffset,
	                            size_t allVerticesCount,
	                            size_t partitionCount)
			: adjacencyList(adjList), localVertexCount(vertexCount), nodeId(nodeId), partitionOffset(partitionOffset),
			  localVertexMaxCount(vertexMaxCount), allVerticesCount(allVerticesCount), partitionCount(partitionCount)
	{
		gIdDatatype = MPI_DATATYPE_NULL;
	};

	MPI_Datatype getGlobalVertexIdDatatype() {
		/* type is registered lazily so that this implementation of GraphPartition can be used without
		 * MPI runtime being initialized (i.e. in tests)
		 */
		if(gIdDatatype == MPI_DATATYPE_NULL) {
			gIdDatatype = ABCPGlobalVertexId<LocalId>::mpiDatatype();
			MPI_Type_commit(&gIdDatatype);
		}

		return gIdDatatype;
	};

	TLocalId toLocalId(const GlobalId gid, VERTEX_TYPE* isLocal = nullptr) {
		if(isLocal != nullptr) *isLocal = (gid.nodeId == nodeId) ? L_MASTER : NON_LOCAL;
		return gid.localId;
	};
	NodeId toMasterNodeId(const GlobalId gid) { return gid.nodeId; };
	GlobalId toGlobalId(TLocalId lid) {
		return GlobalId(nodeId, lid);
	};
	NumericId toNumeric(const GlobalId gid) {
		auto ownersPartitionOffset =
				IndexPartitioner::get_range_for_partition(allVerticesCount, partitionCount, gid.nodeId).first;
		return ownersPartitionOffset + gid.localId;
	};
	NumericId toNumeric(const TLocalId lid) { return partitionOffset + lid; };
	std::string idToString(const GlobalId gid) {
		std::ostringstream os;
		os << "(" << gid.nodeId << "," << gid.localId << ")";
		return os.str();
	};
	std::string idToString(const TLocalId lid) {
		std::ostringstream os;
		os << "(" << nodeId << "," << lid << ")";
		return os.str();
	};
	bool isSame(const GlobalId a, const GlobalId b) {
		return a.nodeId == b.nodeId && a.localId == b.localId;
	};
	bool isValid(const GlobalId id) {
		return id.nodeId >= 0;
	}

	void foreachMasterVertex(std::function<ITER_PROGRESS (const TLocalId)> f) {
		ITER_PROGRESS ip = CONTINUE;
		for(TLocalId vid = 0; vid < localVertexCount && ip == CONTINUE; vid++) {
			ip = f(vid);
		}
	};
	size_t masterVerticesCount() { return localVertexCount; };
	size_t masterVerticesMaxCount() { return localVertexMaxCount; };
	/**
	 * Returns coowners only for masters, not for shadows
	 */
	void foreachCoOwner(TLocalId id, bool returnSelf, std::function<ITER_PROGRESS (const NodeId)> f) {
		if(returnSelf) {
			f(nodeId);
		}
	};
	/**
	 * Works with both masters and shadows
	 */
	void foreachNeighbouringVertex(TLocalId id, std::function<ITER_PROGRESS (const GlobalId)> f) {
		assert(id < localVertexCount);
		auto neighbourList = adjacencyList[id];

		ITER_PROGRESS ip = CONTINUE;
		for(size_t neighId = 0; neighId < neighbourList.size() && ip == CONTINUE; neighId++) {
			ip = f(neighbourList.at(neighId));
		}
	};

	~ArrayBackedChunkedPartition() {
		if(gIdDatatype != MPI_DATATYPE_NULL) {
			MPI_Type_free(&gIdDatatype);
		}
	};

private:
	size_t allVerticesCount;
	size_t partitionCount;

	size_t localVertexCount;
	size_t localVertexMaxCount;
	std::vector<ABCPGlobalVertexId<LocalId>> *adjacencyList;

	NodeId nodeId;
	size_t partitionOffset;

	MPI_Datatype gIdDatatype;
};

template <typename TLocalId, typename TNumId>
class ABCPGraphBuilder : public GraphPartitionHandle<ArrayBackedChunkedPartition<TLocalId, TNumId>> {
private:
	using G = ArrayBackedChunkedPartition<TLocalId, TNumId>;
	IMPORT_ALIASES(G)

public:
	ABCPGraphBuilder(size_t partitionCount, size_t partitionId) : P(partitionCount), partitionId(partitionId) {};

	G* buildGraph(std::string path, std::vector<OriginalVertexId> verticesToConvert) {
		using namespace IndexPartitioner;

		/* read headers to learn how much vertices present */
		AdjacencyListReader<OriginalVertexId> reader(path);
		auto vCount = reader.getVertexCount();

		/* get our range */
		auto range = get_range_for_partition(vCount, P, partitionId);
		auto partitionStart = range.first;
		size_t vertexCount = range.second - range.first;

		// skip vertices that are not our responsibility
		for(int i = 0; i < partitionStart; i++) {
			reader.getNextVertex();
		}

		auto* allLocalVertices = new std::vector<ABCPGlobalVertexId<LocalId>>[vertexCount];
		for(size_t i = 0; i < vertexCount; i++) {
			VertexSpec<OriginalVertexId> vs = *reader.getNextVertex();
			auto idFrom0 = vs.vertexId - partitionStart;
			std::transform(vs.neighbours.begin(), vs.neighbours.end(), std::back_inserter(allLocalVertices[idFrom0]),
			               [=](OriginalVertexId nid) {
				               auto targetPartition = get_partition_from_index(vCount, P, nid);
				               auto targetPartitionStart = get_range_for_partition(vCount, P, targetPartition).first;
				               return ABCPGlobalVertexId<LocalId>(targetPartition,
				                                                  static_cast<TLocalId>(nid) - targetPartitionStart);
			               });
		}

		/* convert vertices */
		convertedVertices.clear();
		for(auto oId: verticesToConvert) {
			int partitionId = IndexPartitioner::get_partition_from_index(reader.getVertexCount(), P, oId);
			int rangeStart = IndexPartitioner::get_range_for_partition(reader.getVertexCount(), P, partitionId).first;
			convertedVertices.push_back(ABCPGlobalVertexId<LocalId>(partitionId, oId - rangeStart));
		}

		/* if anybody gets more than others, it'll be first partition */
		auto longestRange = IndexPartitioner::get_range_for_partition(vCount, P, 0);
		return new G(allLocalVertices, vertexCount, longestRange.second - longestRange.first,
		                                       partitionId, partitionStart, vCount, P);
	};

	std::vector<ABCPGlobalVertexId<LocalId>> getConvertedVertices() {
		return convertedVertices;
	}

	void destroyGraph(const G* g) {
		delete g;
	}

private:
	size_t P;
	size_t partitionId;
	std::vector<ABCPGlobalVertexId<LocalId>> convertedVertices;
};

#endif //FRAMEWORK_SIMPLESTATICGRAPH_H
