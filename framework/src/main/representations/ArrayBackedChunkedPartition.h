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
struct ABCPGlobalVertexId : public GlobalVertexId {
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

template <typename TLocalId, typename TNumId, bool polymorphic>
class ABCPGraphBuilder : public GraphBuilder<ArrayBackedChunkedPartition<TLocalId, TNumId>, polymorphic> {
public:
	ABCPGraphBuilder(size_t partitionCount, size_t partitionId) : P(partitionCount), partitionId(partitionId) {};

	virtual G* buildGraph(std::string path,
	                      std::vector<OriginalVertexId> verticesToConvert) override {
		/* read headers to learn how much vertices present */
		AdjacencyListReader<OriginalVertexId> reader(path);
		auto vCount = reader.getVertexCount();

		/* get our range */
		auto range = IndexPartitioner::get_range_for_partition(vCount, P, partitionId);
		size_t vertexCount = range.second - range.first;

		// skip vertices that are not our responsibility
		for(int i = 0; i < range.first; i++) {
			reader.getVertexCount();
		}

		auto* allLocalVertices = new std::vector<OriginalVertexId>[vertexCount];
		for(size_t i = 0; i < vertexCount; i++) {
			VertexSpec vs = *reader.getNextVertex();
			allLocalVertices[vs.vertexId] = std::vector<OriginalVertexId>(vs.neighbours.begin(), vs.neighbours.end());
		}

		/* convert vertices */
		destroyConvertedVertices();
		for(auto oId: verticesToConvert) {
			int partitionId = IndexPartitioner::get_partition_from_index(reader.getVertexCount(), P, oId);
			int rangeStart = IndexPartitioner::get_range_for_partition(reader.getVertexCount(), P, partitionId).first;
			convertedVertices.push_back(new ABCPGlobalVertexId(partitionId, oId - rangeStart));
		}

		/* if anybody gets more than others, it'll be first partition */
		auto longestRange = IndexPartitioner::get_range_for_partition(vCount, P, 0);
		return new ArrayBackedChunkedPartition(allLocalVertices, vertexCount, longestRange.second - longestRange.first,
		                                       partitionId, range.first);
	};

	virtual std::vector<GlobalVertexId*> getConvertedVertices() override {
		return convertedVertices;
	}

	virtual void destroyConvertedVertices() override {
		if(convertedVertices.size() > 0) {
			for(auto vp: convertedVertices) {
				ABCPGlobalVertexId* vpp = dynamic_cast<ABCPGlobalVertexId*>(vp);
				delete vpp;
			}
			convertedVertices.clear();
		}
	}

	virtual void destroyGraph(const G* g) override {
		delete g;
	}

	virtual ~ABCPGraphBuilder() override {
		destroyConvertedVertices();
	}

private:
	size_t P;
	size_t partitionId;
	std::vector<GlobalVertexId*> convertedVertices;
};

/*
 * Each issues GlobalVertexId must be acccount for using freeGlobalId (excluding those returned during
 * internal iteration - those should never be freed)
 * Lifetime of each GlobalId is tied to lifetime of ABCP class which returned it (storing it somewhere is not a
 * good idea)
 */
template <typename TBase, typename TLocalId, typename TNumId>
class ArrayBackedChunkedPartition : public TBase {
public:
	ArrayBackedChunkedPartition(std::vector<ABCPGlobalVertexId> *adjList,
	                            size_t vertexCount,
	                            size_t vertexMaxCount,
	                            NodeId nodeId,
	                            size_t partitionOffset)
			: adjacencyList(adjList), vertexCount(vertexCount), nodeId(nodeId), partitionOffset(partitionOffset),
			  vertexMaxCount(vertexMaxCount)
	{
		gIdDatatype = MPI_DATATYPE_NULL;
	};

	virtual MPI_Datatype geGlobalVertexIdDatatype() {
		/* type is registered lazily so that this implementation of GraphPartition can be used without
		 * MPI runtime being initialized (i.e. in tests)
		 */
		if(gIdDatatype == MPI_DATATYPE_NULL) {
			gIdDatatype = ABCPGlobalVertexId::mpiDatatype();
			MPI_Type_commit(&gIdDatatype);
		}

		return gIdDatatype;
	};

	virtual TLocalId toLocalId(const GlobalVertexId& gid) { return c(gid).localId; };
	virtual NodeId toMasterNodeId(const GlobalVertexId& gid) { return c(gid).nodeId; };
	virtual GlobalVertexId& toGlobalId(TLocalId lid) {
		auto gid = gidPool.malloc();
		gid->nodeId = lid;
		gid->nodeId = nodeId;
		return *gid;
	};
	virtual void freeGlobalId(const GlobalVertexId& gid) {
		/* all ABCPGlobalVertexId's are issued by us, they are not const so stripping it should not invoke
		 * undefined behaviour */
		const ABCPGlobalVertexId& ourRef = c(gid);
		auto ourNonConst = const_cast<ABCPGlobalVertexId&>(ourRef);
		gidPool.free(&ourNonConst);
	};
	virtual TNumericId toNumeric(const GlobalVertexId& gid) { return partitionOffset + c(gid).localId; };
	virtual std::string idToString(const GlobalVertexId& gid) {
		std::ostringstream os;
		auto g = c(gid);
		os << "(" << g.nodeId << "," << g.localId << ")";
		return os.str();
	};
	virtual bool isSame(const GlobalVertexId& id0, const GlobalVertexId& id1) {
		auto a = c(id0);
		auto b = c(id1);
		return a.nodeId == b.nodeId && a.localId == b.localId;
	};

	virtual void foreachMasterVertex(std::function<bool(const TLocalId)> f) {
		bool shouldStop = false;
		for(TLocalId vid = 0; vid < vertexCount && !shouldStop; vid++) {
			shouldStop = f(vid);
		}
	};
	virtual size_t masterVerticesCount() { return vertexCount; };
	virtual size_t masterVerticesMaxCount() { return vertexMaxCount; };
	/**
	 * Returns coowners only for masters, not for shadows
	 */
	virtual void foreachCoOwner(TLocalId id, bool returnSelf, std::function<bool(const NodeId)> f) {
		if(returnSelf) {
			f(nodeId);
		}
	};
	/**
	 * Works with both masters and shadows
	 */
	virtual void foreachNeighbouringVertex(TLocalId id, std::function<bool(const GlobalVertexId&)> f) {
		assert(id < vertexCount);
		auto neighbourList = adjacencyList[id];

		bool shouldStop = false;
		for(size_t neighId = 0; neighId < neighbourList.size() && !shouldStop; neighId++) {
			shouldStop = f(neighbourList.at(neighId));
		}
	};

	virtual ~GraphPartition() {
		if(gIdDatatype != MPI_DATATYPE_NULL) {
			MPI_Type_free(&gIdDatatype);
		}
	};

private:
	size_t vertexCount;
	size_t vertexMaxCount;
	std::vector<ABCPGlobalVertexId> *adjacencyList;

	NodeId nodeId;
	size_t partitionOffset;

	MPI_Datatype gIdDatatype;
	boost::object_pool<ABCPGlobalVertexId> gidPool;

private:
	const ABCPGlobalVertexId& c(const GlobalVertexId& v) {
		return dynamic_cast<const ABCPGlobalVertexId<TLocalId>&>(v);
	}
};

#endif //FRAMEWORK_SIMPLESTATICGRAPH_H
