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

enum ITER_PROGRESS {
	STOP,
	CONTINUE,
};

/**
 * Any method of this class (including special members) is guaranteed to be executed while MPI is up-and-working
 *
 * In a generic partitioning, both edge and vertex can span mulitple nodes. What does it mean?
 * - edge - linked vertices stored on different nodes, so must be referenced twice (and so it's data)
 * - vertex - information about neighbours spread across nodes
 *	(C) Even with edge-centric approach to partitioning, storing them in edge based format might not be the best idea.
 *		Since vertices ususally have more than one neighbour, by storing them in vertex oriented format we save more
 *		space than when storing them in edge-orinted format.
 *
 * In vertex-centric approach we cut edges. In edge-centric we cut vertices (spread information across nodes).
 *
 * This is a vertex centric interface. Another one (edge-centric) should be provided at some point.
 * However, to account for different partitionings, we assume that each vertex can be "split" - information
 * about neighbours can be stored on more than one node.
 * In vertex-centric interface edges does not have separate IDs - they can be only accessed by quering associated
 * vertices.
 *
 * Each vertex/edge has one master copy and some arbitrary amount of slaves/shadows.
 * 	(C) I haven't decide yet about what to do with data associated with vertices - but if I decide to add it to the
 * 		interface:
 * 		- we can expose read-only copies on slaves, but all writes should probably be directed to master
 * 		and replicated from there.
 * 		- modifier can simply broadcast change - master-master replication ususally discouraged in distributed systems,
 * 		but here we are working with reliable network, so such an approach might work fine
 *
 *
 * We need to know who stores the neighbourship information for given vertex. However, to reduce storage overhead,
 * only master keeps track of that information, shadows don't know it.
 * (of course representation can pick some regular partitioning, in which this can be determined in O(1) instead
 * of consulting some kind of mapping).
 *
 * Each shadow should know where his master is stored (however, it's not required to know about other shadows).
 *
 * Each vertex is identified by GlobalId (valid across all nodes) and LocalId (valid only on local node). Shadows
 * have identical GlobalId to master's, but LocalIDs can be different.
 *  (C) LocalIDs introduced in order to make storage of metadata predictable - you can use simple array. This is
 *  	especially important when using RMA - alternative would be to create distributed hash table, so that
 *  	any node can compute hash (and thus expected location) of any key. If we add hashing conflicts to that mix, we
 *  	may end up with a lot of RMA requests.
 *
 * Each vertex stored on local machine must have LocalId assigned (masters and shadows from other nodes). However,
 * all shadows should have ids larger than any master (first masters, then shadows). There can be a gap between
 * IDs for masters and IDs for shadows.
 * 	(C) This may seem problematic, because:
 * 		- we have to know in advance how many master nodes'll be assigned to a given node, or
 * 		- we need to reorganize data later on, or
 * 		- we have to cache shadow data and write it at the end
 * 		We use internal iteration, so we could probably waive it, at a cost of:
 * 		- interspersing master nodes with shadow nodes, or
 * 		- creating separate API to access shadows
 * 		Note - we don't have to store masters and shadows in the same memory area. We can use two instead and
 * 		only mimic single LocalId addressing space
 *
 * GlobalId is an opaque object (implementation dependent), LocalId is integer value.
 * LocalIds occupy continous space.
 * GlobalId can be converted into unique integer (but the range is not guaranteed to be continous) /look up pairing
 * functions /
 *
 * Neither localId/nodeId pair nor numeric representation are required to be identical during different executions.
 *
 * Representation must be able to convert GlobalId of any node stored locally to it's corresponding LocalId or give
 * information, that node is not stored locally.
 * Any LocalId can be converted to corresponding GlobalId (both masters and shadows) (of course only on 'this' node).
 * 	(C) Isn't this in direct contradiction to the desire to store mapping data only with master copy? Can we perform
 * 		that conversion without mapping information?
 *
 * TGlobalId type parameter is set by subclass (to force presence of proper typedef), rest is set by end-user
 *
 * Default constructor of TGlobalId should create invalid vertex - valid ones can be returned only
 * from the GraphPartition.
 *
 * Open questions:
 * - does this representation work for both directed and undirected graphs?
 * - some kind of capability system needed - otherwise even when writing simplest algorithms we have to assume that
 * 	vertices can be spread and do forEachCoOwner...
 *
 *
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



	void foreachMasterVertex(std::function<ITER_PROGRESS (const TLocalId)>);
	size_t masterVerticesCount();
	size_t masterVerticesMaxCount();

	void foreachShadowVertex(std::function<ITER_PROGRESS (const TLocalId, const TGlobalId)>);
	size_t shadowVerticesCount();
	/**
	 * Returns coowners only for masters, not for shadows
	 */
	void foreachCoOwner(TLocalId, bool returnSelf, std::function<ITER_PROGRESS (const NodeId)>);
	/**
	 * Works with both masters and shadows
	 */
	void foreachNeighbouringVertex(TLocalId, std::function<ITER_PROGRESS (const TGlobalId)>);

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
	void foreachMasterVertex(std::function<ITER_PROGRESS (const LocalId)>) {}
	size_t masterVerticesCount() {return 0;}
	size_t masterVerticesMaxCount() {return 0;}
	void foreachCoOwner(LocalId, bool returnSelf, std::function<ITER_PROGRESS (const NodeId)>) {}
	void foreachNeighbouringVertex(LocalId, std::function<ITER_PROGRESS (const GlobalId)>) {}
};

#endif //FRAMEWORK_GRAPH_H

