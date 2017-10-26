
#include <gtest/gtest.h>
#include <mpi.h>
#include <utils/TestUtils.h>
#include <Runner.h>
#include <representations/AdjacencyListHashPartition.h>
#include <algorithms/colouring/GraphColouringMp.h>
#include <algorithms/colouring/GraphColouringMpAsync.h>
#include <validators/ColouringValidator.h>

template <
		template<typename, typename> class TGraphBuilder,
		template<typename> class TAlgo,
		template<typename> class TValidator>
static void executeTest(std::string graphPath)
{
	auto *graphBuilder = new TGraphBuilder<int,int>();
	auto *g = graphBuilder->buildGraph(graphPath);
	delete graphBuilder;

	using TGP = typename TGraphBuilder<int,int>::GPType;
	using TResult = typename TAlgo<TGP>::ResultType;

	auto* algo = new TAlgo<TGP>();
	auto* validator = new TValidator<TGP>();
	AlgorithmExecutionResult r = runAndCheck(g, *algo, *validator);

	delete validator;
	delete algo;
	graphBuilder->destroyGraph(g);
	ASSERT_TRUE(r.algorithmStatus);
	ASSERT_TRUE(r.validatorStatus);
}

template <template<typename> class TAlgo>
using executeColouringTest = decltype(executeTest<ALHGraphHandle, TAlgo, ColouringValidator>(""));

TEST(ColouringMPAsync, FindsCorrectSolutionForSTG) {
	executeColouringTest<GraphColouringMPAsync>("resources/test/SimpleTestGraph.adjl");
}

TEST(ColouringMPAsync, FindsCorrectSolutionForComplete50) {
	executeColouringTest<GraphColouringMPAsync>("resources/test/complete50.adjl");
}

TEST(ColouringMP, FindsCorrectSolutionForSTG) {
	executeColouringTest<GraphColouringMp>("resources/test/SimpleTestGraph.adjl");
}

TEST(ColouringMP, FindsCorrectSolutionForComplete50) {
	executeColouringTest<GraphColouringMp>("resources/test/complete50.adjl");
}