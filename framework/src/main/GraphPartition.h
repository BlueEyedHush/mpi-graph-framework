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
	MPI_Datatype getGlobalVertexIdDatatype();

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
	std::string idToString(const TLocalId);
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
#define IMPORT_ALIASES(ALIAS_HOLDER) \
	using LocalId = typename ALIAS_HOLDER::LidType; \
	using GlobalId = typename ALIAS_HOLDER::GidType; \
	using NumericId = typename ALIAS_HOLDER::NumType; \

/* specialization that can be used when writing algorithms (doesn't expose anything /e.g. like GlobalVertexId insides
 * apart from what's present in the interface) */
struct TGVID {};

class TestGP : public GraphPartition<TGVID, unsigned int, unsigned int> {
private:
	using P = GraphPartition<TGVID, unsigned int, unsigned int>;
	IMPORT_ALIASES(P)

public:
	TestGP() {}
	MPI_Datatype getGlobalVertexIdDatatype() {return MPI_DATATYPE_NULL;return 0;}
	LocalId toLocalId(const GlobalId, VERTEX_TYPE* vtype = nullptr) {return 0;}
	NodeId toMasterNodeId(const GlobalId) {return -1;}
	GlobalId toGlobalId(const LocalId) {return TGVID();}
	NumericId toNumeric(const GlobalId) {return 0;}
	NumericId toNumeric(const LocalId) {return 0;}
	std::string idToString(const GlobalId) {return "";}
	std::string idToString(const LocalId) {return "";}
	bool isSame(const GlobalId, const GlobalId) {return false;}
	bool isValid(const GlobalId) {return false;}
	void foreachMasterVertex(std::function<bool(const LocalId)>) {}
	size_t masterVerticesCount() {return 0;}
	size_t masterVerticesMaxCount() {return 0;}
	void foreachCoOwner(LocalId, bool returnSelf, std::function<bool(const NodeId)>) {}
	void foreachNeighbouringVertex(LocalId, std::function<bool(const GlobalId)>) {}
};

#endif //FRAMEWORK_GRAPH_H

