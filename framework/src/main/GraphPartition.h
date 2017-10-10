//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPH_H
#define FRAMEWORK_GRAPH_H

#include <cinttypes>

#define LOCAL_VERTEX_ID_MPI_TYPE MPI_UNSIGNED_LONG_LONG;
typedef unsigned long long LocalVertexId;
#define NUMID_MPI_TYPE MPI_UNSIGNED_LONG_LONG;
typedef unsigned long long NumericIdRepr;
/* for the lack of better place it goes here (but should be in some global header */
#define NODE_ID_MPI_TYPE MPI_INT;
typedef int NodeId;

struct GlobalVertexId {
	virtual bool isValid() = 0;
	virtual std::string toString() = 0;
};

template <class T>
class Iterator {
	virtual T operator*() = 0;
	virtual void operator++() = 0;
	virtual bool operator!=(const Iterator&) = 0;
};

template <class T>
class IteratorContainer {
public:
	virtual Iterator<T> begin() = 0;
	virtual size_t size() = 0;
	virtual Iterator<T> end() = 0;
};

class MasterVerticesList {
	virtual Iterator<LocalVertexId> begin() = 0;
	virtual Iterator<LocalVertexId> end() = 0;
	virtual size_t size() = 0;
	virtual size_t maxSize() = 0;
};

/**
 * Any method of this class (including special members) is guaranteed to be executed while MPI is up-and-working
 *
 * GraphPartition object retains ownership of all returned GlobalVertexId. Each no longer necessary GlobalVertexId
 * object should be explicitly released using free(GlobalVertexId&)
 */
class GraphPartition {
public:

	/**
	 * Datatypes should be registers with MPI upon construction and deregistered during destruction
	 */
	virtual MPI_Datatype getGlobalVertexIdDatatype() = 0;

	virtual LocalVertexId toLocalId(const GlobalVertexId&) = 0;
	virtual NodeId toMasterNodeId(const GlobalVertexId&) = 0;
	virtual GlobalVertexId& toGlobalId(LocalVertexId) = 0;
	virtual void free(const GlobalVertexId&) = 0;
	virtual NumericIdRepr toNumeric(const GlobalVertexId&) = 0;

	/* aliased by default, but can be overridden */
	NumericIdRepr toNumeric(LocalVertexId lvid) {
		auto gvid = toGlobalId(lvid);
		auto numeric = toNumeric(gvid);
		free(gvid);
		return numeric;
	}

	virtual MasterVerticesList getMasterVertices() = 0;
	/**
	 * Returns coowners only for masters, not for shadows
	 */
	virtual IteratorContainer<NodeId> getCoOwners(LocalVertexId) = 0;
	/**
	 * Works with both masters and shadows
	 */
	virtual IteratorContainer<GlobalVertexId> getNeighbouringVertices(LocalVertexId) = 0;

	virtual ~GraphPartition() {};
};

#endif //FRAMEWORK_GRAPH_H
