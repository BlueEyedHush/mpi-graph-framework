//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPH_H
#define FRAMEWORK_GRAPH_H

#include <iostream>
#include <functional>
#include <mpi.h>


/**
 * Representation
 */

#define NODE_ID_MPI_TYPE MPI_INT;
typedef int NodeId;

/* if one wants to use GlobalVertexId without virtual overhead, he probably has to write it exacly the same way
 * concrete implementations are written - parametrize base
 */
struct GlobalVertexId {
	virtual bool isValid() = 0;
	virtual std::string toString() = 0;
};

/**
 * Any method of this class (including special members) is guaranteed to be executed while MPI is up-and-working
 *
 * GraphPartition object retains ownership of all returned GlobalVertexId. Each no longer necessary GlobalVertexId
 * object should be explicitly released using free(GlobalVertexId&)
 */
template <typename TLocalId, typename TNumericId>
class GraphPartition {
public:
	typedef TLocalId LidType;
	typedef TNumericId NumType;

public:

	/**
	 * Datatypes should be registers with MPI upon construction and deregistered during destruction
	 */
	virtual MPI_Datatype geGlobalVertexIdDatatype() = 0;

	virtual TLocalId toLocalId(const GlobalVertexId&) = 0;
	virtual NodeId toMasterNodeId(const GlobalVertexId&) = 0;
	virtual GlobalVertexId& toGlobalId(TLocalId) = 0;
	virtual void freeGlobalId(const GlobalVertexId&) = 0;
	virtual TNumericId toNumeric(const GlobalVertexId&) = 0;

	/* aliased by default, but can be overridden */
	TNumericId toNumeric(TLocalId lvid) {
		auto gvid = toGlobalId(lvid);
		auto numeric = toNumeric(gvid);
		freeGlobalId(gvid);
		return numeric;
	}

	virtual void foreachMasterVertex(std::function<bool(const TLocalId&)>) = 0;
	virtual size_t masterVerticesCount() = 0;
	virtual size_t masterVerticesMaxCount() = 0;
	/**
	 * Returns coowners only for masters, not for shadows
	 */
	virtual void foreachCoOwner(TLocalId, std::function<bool(const NodeId&)>) = 0;
	/**
	 * Works with both masters and shadows
	 */
	virtual void foreachNeighbouringVertex(TLocalId, std::function<bool(const GlobalVertexId&)>) = 0;

	virtual ~GraphPartition() {};
};

/*
 * defines and typedefs that are used as template arguments when we intend to leverage polymorphism
 */
#define LOCAL_VERTEX_ID_MPI_TYPE MPI_UNSIGNED_LONG_LONG;
typedef unsigned long long LocalVertexId;
#define NUMID_MPI_TYPE MPI_UNSIGNED_LONG_LONG;
typedef unsigned long long NumericIdRepr;

typedef GraphPartition<LocalVertexId, NumericIdRepr> DGraphPartition;

/*
 * Usually, GraphPartition is the parent of concrete representation. If we want to avoid overhead of
 * virtuals (at the ccost of lack of flexibility), we can use this class as base.
 */
template <typename TLocalId, typename TNumericId>
class DummyGraphPartition {
public:
	typedef TLocalId LidType;
	typedef TNumericId NumType;
};



#endif //FRAMEWORK_GRAPH_H
