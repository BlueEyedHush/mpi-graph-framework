//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_SIMPLESTATICGRAPH_H
#define FRAMEWORK_SIMPLESTATICGRAPH_H

#include <algorithm>
#include <climits>
#include <sstream>
#include <boost/pool/object_pool.hpp>
#include <GraphPartition.h>
#include <GraphBuilder.h>
#include <utils/IndexPartitioner.h>
#include <utils/AdjacencyListReader.h>
#include <utils/MpiTypemap.h>

template <typename T>
struct ABCPGlobalVertexId {
	ABCPGlobalVertexId() {}
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
};

template <typename TLocalId, typename TNumId>
class ArrayBackedChunkedPartition;

template <typename TLocalId, typename TNumId>
class ABCPGraphBuilder : public GraphBuilder<ArrayBackedChunkedPartition<TLocalId, TNumId>> {
private:
	typedef ArrayBackedChunkedPartition<TLocalId, TNumId> G;

public:
	ABCPGraphBuilder(size_t partitionCount, size_t partitionId) : P(partitionCount), partitionId(partitionId) {};

	G* buildGraph(std::string path, std::vector<OriginalVertexId> verticesToConvert) {
		/* read headers to learn how much vertices present */
		AdjacencyListReader<OriginalVertexId> reader(path);
		auto vCount = reader.getVertexCount();

		/* get our range */
		auto range = IndexPartitioner::get_range_for_partition(vCount, P, partitionId);
		auto partitionStart = range.first;
		size_t vertexCount = range.second - range.first;

		// skip vertices that are not our responsibility
		for(int i = 0; i < range.first; i++) {
			reader.getVertexCount();
		}

		auto* allLocalVertices = new std::vector<ABCPGlobalVertexId>[vertexCount];
		for(size_t i = 0; i < vertexCount; i++) {
			VertexSpec<OriginalVertexId> vs = *reader.getNextVertex();
			std::transform(vs.neighbours.begin(), vs.neighbours.end(), std::back_inserter(allLocalVertices[vs.vertexId]),
			               [=](OriginalVertexId nid) {
				               return ABCPGlobalVertexId(partitionId, static_cast<TLocalId>(nid) - partitionStart);
			               });
		}

		/* convert vertices */
		convertedVertices.clear();
		for(auto oId: verticesToConvert) {
			int partitionId = IndexPartitioner::get_partition_from_index(reader.getVertexCount(), P, oId);
			int rangeStart = IndexPartitioner::get_range_for_partition(reader.getVertexCount(), P, partitionId).first;
			convertedVertices.push_back(ABCPGlobalVertexId(partitionId, oId - rangeStart));
		}

		/* if anybody gets more than others, it'll be first partition */
		auto longestRange = IndexPartitioner::get_range_for_partition(vCount, P, 0);
		return new ArrayBackedChunkedPartition(allLocalVertices, vertexCount, longestRange.second - longestRange.first,
		                                       partitionId, partitionStart, vCount, P);
	};

	std::vector<ABCPGlobalVertexId> getConvertedVertices() {
		return convertedVertices;
	}

	void destroyGraph(const G* g) {
		delete g;
	}

private:
	size_t P;
	size_t partitionId;
	std::vector<ABCPGlobalVertexId> convertedVertices;
};

/*
 * Each issues GlobalVertexId must be acccount for using freeGlobalId (excluding those returned during
 * internal iteration - those should never be freed)
 * Lifetime of each GlobalId is tied to lifetime of ABCP class which returned it (storing it somewhere is not a
 * good idea)
 *
 * For this representation, toNumeric always returns original ID (as loaded from file).
 */
template <typename TLocalId, typename TNumId>
class ArrayBackedChunkedPartition : public GraphPartition<ABCPGlobalVertexId, TLocalId, TNumId> {
public:
	ArrayBackedChunkedPartition(std::vector<ABCPGlobalVertexId> *adjList,
	                            size_t vertexCount,
	                            size_t vertexMaxCount,
	                            NodeId nodeId,
	                            size_t partitionOffset,
	                            size_t allVerticesCount,
	                            size_t partitionCount)
			: adjacencyList(adjList), localVertexCount(vertexCount), nodeId(nodeId), partitionOffset(partitionOffset),
			  localVertexMaxCount(vertexMaxCount)
	{
		gIdDatatype = MPI_DATATYPE_NULL;
	};

	MPI_Datatype geGlobalVertexIdDatatype() {
		/* type is registered lazily so that this implementation of GraphPartition can be used without
		 * MPI runtime being initialized (i.e. in tests)
		 */
		if(gIdDatatype == MPI_DATATYPE_NULL) {
			gIdDatatype = ABCPGlobalVertexId::mpiDatatype();
			MPI_Type_commit(&gIdDatatype);
		}

		return gIdDatatype;
	};

	TLocalId toLocalId(const GidType gid, VERTEX_TYPE* isLocal = nullptr) {
		if(isLocal != nullptr) *isLocal = (gid.nodeId == nodeId) ? L_MASTER : NON_LOCAL;
		return gid.localId;
	};
	NodeId toMasterNodeId(const GidType gid) { return gid.nodeId; };
	GidType toGlobalId(TLocalId lid) {
		return GidType(nodeId, lid);
	};
	NumType toNumeric(const GidType gid) {
		auto ownersPartitionOffset =
				IndexPartitioner::get_range_for_partition(allVerticesCount, partitionCount, nodeId).first;
		return ownersPartitionOffset + gid.localId;
	};
	NumType toNumeric(const TLocalId lid) { return partitionOffset + lid; };
	std::string idToString(const GidType gid) {
		std::ostringstream os;
		os << "(" << gid.nodeId << "," << gid.localId << ")";
		return os.str();
	};
	std::string idToString(const TLocalId lid) {
		std::ostringstream os;
		os << "(" << nodeId << "," << lid << ")";
		return os.str();
	};
	bool isSame(const GidType a, const GidType b) {
		return a.nodeId == b.nodeId && a.localId == b.localId;
	};

	void foreachMasterVertex(std::function<bool(const TLocalId)> f) {
		bool shouldStop = false;
		for(TLocalId vid = 0; vid < localVertexCount && !shouldStop; vid++) {
			shouldStop = f(vid);
		}
	};
	size_t masterVerticesCount() { return localVertexCount; };
	size_t masterVerticesMaxCount() { return localVertexMaxCount; };
	/**
	 * Returns coowners only for masters, not for shadows
	 */
	void foreachCoOwner(TLocalId id, bool returnSelf, std::function<bool(const NodeId)> f) {
		if(returnSelf) {
			f(nodeId);
		}
	};
	/**
	 * Works with both masters and shadows
	 */
	void foreachNeighbouringVertex(TLocalId id, std::function<bool(const typename GidType)> f) {
		assert(id < localVertexCount);
		auto neighbourList = adjacencyList[id];

		bool shouldStop = false;
		for(size_t neighId = 0; neighId < neighbourList.size() && !shouldStop; neighId++) {
			shouldStop = f(neighbourList.at(neighId));
		}
	};

	~GraphPartition() {
		if(gIdDatatype != MPI_DATATYPE_NULL) {
			MPI_Type_free(&gIdDatatype);
		}
	};

private:
	size_t allVerticesCount;
	size_t partitionCount;

	size_t localVertexCount;
	size_t localVertexMaxCount;
	std::vector<ABCPGlobalVertexId> *adjacencyList;

	NodeId nodeId;
	size_t partitionOffset;

	MPI_Datatype gIdDatatype;
};

#endif //FRAMEWORK_SIMPLESTATICGRAPH_H
