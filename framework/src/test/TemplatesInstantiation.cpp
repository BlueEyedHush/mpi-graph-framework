

/**
 * Check correctness of templates by trying to instantiate them
 *
 * This code is never executed, it exist only to allow compiler to inspect template specialization.
 */

/* for types which doesn't have default constructor; since we are never going to execute this,
 * it's ugliness shouldn't be a problem
 */
template <typename T>
T* uglyInstantiation() {
	auto* t = new char[sizeof(T)];
	return reinterpret_cast<T*>(t);
}

/*
 * Builders and representations
 */
#include <representations/ArrayBackedChunkedPartition.h>
using ABCP_GB = ABCGraphHandle<int,int>;
using ABCP_GP = ArrayBackedChunkedPartition<int,int>;
using ABCP_GB_U = ABCGraphHandle<size_t,size_t>;
using ABCP_GP_U = ArrayBackedChunkedPartition<size_t,size_t>;

#include <representations/AdjacencyListHashPartition.h>
using ALHP_GB = ALHGraphHandle<int,int>;
using ALHP_GP = ALHPGraphPartition<int,int>;
using ALHP_GB_U = ALHGraphHandle<size_t,size_t>;
using ALHP_GP_U = ALHPGraphPartition<size_t,size_t>;

#include <representations/RoundRobin2DPartition.h>
using RR2D_GB = RR2DHandle<int,int>;
using RR2D_GP = RoundRobin2DPartition<int,int>;
using RR2D_GB_U = RR2DHandle<size_t,size_t>;
using RR2D_GP_U = RoundRobin2DPartition<size_t,size_t>;

template <typename TGraphBuilder>
void callEachGhFunction(TGraphBuilder* builder) {
	builder->getGraph();
	builder->getConvertedVertices();
	builder->releaseGraph();
}

template <typename TGraphPartition>
void callEachGpFunction(TGraphPartition* gp) {
	IMPORT_ALIASES(TGraphPartition)

	GlobalId globalId;
	LocalId localId = 0;
	VERTEX_TYPE vtype;
	auto nodeIdConsumer = [](const NodeId) {return ITER_PROGRESS::STOP;};
	auto localIdconsumer = [](const LocalId) {return ITER_PROGRESS::STOP;};
	auto globalIdconsumer = [](const GlobalId) {return ITER_PROGRESS::STOP;};
	auto localAndGlobalIdConsumer = [](const LocalId, const GlobalId) {return ITER_PROGRESS::STOP;};

	gp->getGlobalVertexIdDatatype();
	gp->toLocalId(globalId, &vtype);
	gp->toMasterNodeId(globalId);
	gp->toGlobalId(localId);
	gp->toNumeric(globalId);
	gp->toNumeric(localId);
	gp->idToString(globalId);
	gp->idToString(localId);
	gp->isSame(globalId, globalId);
	gp->isValid(globalId);
	gp->foreachMasterVertex(localIdconsumer);
	gp->masterVerticesCount();
	gp->masterVerticesMaxCount();
	gp->foreachCoOwner(localId, true, nodeIdConsumer);
	gp->foreachNeighbouringVertex(localId, globalIdconsumer);
	gp->foreachShadowVertex(localAndGlobalIdConsumer);
	gp->shadowVerticesCount();
}

void testBuildersAndGraphs() {
	auto* adjList = new std::vector<TGVID>();
	callEachGpFunction(new ABCP_GP(new std::vector<ABCP_GP::GidType>(), 0, 0, 0, 0, 0, 0));
	callEachGpFunction(new ABCP_GP_U(new std::vector<ABCP_GP_U::GidType>(), 0, 0, 0, 0, 0, 0));
	callEachGpFunction(new ALHP_GP(details::GraphData<int, ALHP_GP::GidType>()));
	callEachGpFunction(new ALHP_GP_U(details::GraphData<size_t, ALHP_GP_U::GidType>()));
	details::RR2D::MpiTypes mtypes;
	callEachGpFunction(new RR2D_GP(new details::RR2D::GraphData<int, int>(0, 0, 0, 0, 0, 0, 0, mtypes)));
	callEachGpFunction(new RR2D_GP_U(new details::RR2D::GraphData<size_t, size_t>(0, 0, 0, 0, 0, 0, 0, mtypes)));

	auto vToConv = std::vector<OriginalVertexId>();
	callEachGhFunction(new ABCP_GB("", 0, 0, vToConv));
	callEachGhFunction(new ABCP_GB_U("", 0, 0, vToConv));
	callEachGhFunction(new ALHP_GB("", vToConv));
	callEachGhFunction(new ALHP_GB_U("", vToConv));
	callEachGhFunction(new RR2D_GB("", vToConv));
	callEachGhFunction(new RR2D_GB_U("", vToConv));
}

/*
 * Algorithms
 */
#include <algorithms/bfs/BfsVarMessage.h>
using BFS_VM = Bfs_Mp_VarMsgLen_1D_2CommRounds<TestGP>;

#include <algorithms/bfs/BfsFixedMessage.h>
using BFS_FM = Bfs_Mp_FixedMsgLen_1D_2CommRounds<TestGP>;

#include <algorithms/bfs/Bfs1CommsRound.h>
using BFS_1C = Bfs_Mp_VarMsgLen_1D_1CommsTag<TestGP>;

#include <algorithms/colouring/GraphColouringMp.h>
using COLOUR_MP = GraphColouringMp<TestGP>;

#include <algorithms/colouring/GraphColouringMpAsync.h>
using COLOUR_MP_ASYNC = GraphColouringMPAsync<TestGP>;


template <typename TAlgo> void callEachAlgoFunctions(TAlgo* algo) {
	auto G = TestGP();
	algo->run(&G, AAuxiliaryParams());
	algo->getResult();
}

void testAlgorithms() {
	auto bfsRoot = TGVID();
	callEachAlgoFunctions(new BFS_VM(bfsRoot));
	callEachAlgoFunctions(new BFS_FM(bfsRoot));
	callEachAlgoFunctions(new BFS_1C(bfsRoot));

	callEachAlgoFunctions(new COLOUR_MP());
	callEachAlgoFunctions(new COLOUR_MP_ASYNC());
}

/*
 * Validators
 */
#include <validators/BfsValidator.h>
using V_BFS = BfsValidator<TestGP>;

#include <validators/ColouringValidator.h>
using V_COLOUR = ColouringValidator<TestGP>;

template <typename TValidator>
void callEachValidatorFunctions(TValidator* algo) {
	auto G = TestGP();
	using R = typename TValidator::ResultType;
	auto result = *uglyInstantiation<R>();
	algo->validate(&G, result);
}

void testValidators() {
	auto bfsRoot = TGVID();
	callEachValidatorFunctions(new V_BFS(bfsRoot));
	callEachValidatorFunctions(new V_COLOUR());
}