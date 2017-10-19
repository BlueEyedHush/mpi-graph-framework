//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPH_H
#define FRAMEWORK_GRAPH_H

#include <iostream>
#include <functional>
#include <mpi.h>
#include <Prerequisites.h>

/**
 * Representation
 */

enum VERTEX_TYPE {
	L_MASTER,
	L_SHADOW,
	NON_LOCAL
};

/**
 * Any method of this class (including special members) is guaranteed to be executed while MPI is up-and-working
 *
 * GraphPartition object retains ownership of all returned GlobalVertexId. Each no longer necessary GlobalVertexId
 * object should be explicitly released using free(GlobalVertexId&)
 *
 * Each vertex stored on local machine must have LocalId assigned (masters and shadows from other nodes). However,
 * all shadows should have ids larger than any master (first masters, then shadows).
 *
 * Neither localId/nodeId pair nor numeric representation are required to be identical during different executions.
 * 
 * TGlobalId type parameter is set by subclass (to force presence of proper typedef), rest is set by end-user
 *
 * Default constructor of TGlobalId should create invalid vertex - valid ones can be returned only
 * from the GraphPartition.
 */
template <typename TGlobalId, typename TLocalId, typename TNumericId>
class GraphPartition {
public:
	typedef TLocalId LidType;
	typedef TNumericId NumType;
	typedef TGlobalId GidType;

public:

	/**
	 * Datatypes should be registers with MPI upon construction and deregistered during destruction
	 * In particular, this helper shouldn't register the datatype -> it can be called more than once
	 */
	MPI_Datatype geGlobalVertexIdDatatype();

	/**
	 * Works only for locally stored vertices: masters and shadows (slaves)
	 * If non-local vertex is passed, rubbish'll be returned and isLocal output parameter
	 * will be set to true (if caller is not interested in isLocal value, he/she can simply pass nullptr)
	 *
	 */
	TLocalId toLocalId(const TGlobalId, VERTEX_TYPE* vtype = nullptr);
	NodeId toMasterNodeId(const TGlobalId);
	TGlobalId toGlobalId(const TLocalId);
	TNumericId toNumeric(const TGlobalId);
	TNumericId toNumeric(const TLocalId);
	std::string idToString(const TGlobalId);
	std::string idToString(const TLocalId lId);
	bool isSame(const TGlobalId, const TGlobalId);
	bool isValid(const TGlobalId);



	void foreachMasterVertex(std::function<bool(const TLocalId)>);
	size_t masterVerticesCount();
	size_t masterVerticesMaxCount();
	/**
	 * Returns coowners only for masters, not for shadows
	 */
	void foreachCoOwner(TLocalId, bool returnSelf, std::function<bool(const NodeId)>);
	/**
	 * Works with both masters and shadows
	 */
	void foreachNeighbouringVertex(TLocalId, std::function<bool(const TGlobalId)>);

protected:
	/* to prevent anybody from using this class as more than reference */
	GraphPartition() {}
	~GraphPartition() {};
};

/* macros that can be used in classes parametrized by GraphPartition */
#define GP_TYPEDEFS \
	typedef typename TGraphPartition::LidType LocalId; \
	typedef typename TGraphPartition::NumType NumericId; \
	typedef typename TGraphPartition::GidType GlobalId;

/* specialization that can be used when writing algorithms (doesn't expose anything /e.g. like GlobalVertexId insides
 * apart from what's present in the interface) */
struct TGVID {};
typedef GraphPartition<TGVID, unsigned int, unsigned int> TestGP;

#endif //FRAMEWORK_GRAPH_H

