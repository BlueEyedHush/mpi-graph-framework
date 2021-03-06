
#include <gtest/gtest.h>
#include <mpi.h>
#include <utils/TestUtils.h>
#include <Executor.h>
#include <Assembly.h>
#include <representations/AdjacencyListHashPartition.h>
#include <algorithms/colouring/GraphColouringMp.h>
#include <algorithms/colouring/GraphColouringMpAsync.h>
#include <assemblies/ColouringAssembly.h>

using GH = ALHGraphHandle<int, int>;

template <typename TGraphBuilder, template<typename> class TAlgo>
static void executeTest(std::string graphPath)
{
	ConfigMap cm;
	cm.emplace(GH::E_DIV_OPT, "1");
	cm.emplace(GH::V_DIV_OPT, "1");

	GBAuxiliaryParams auxParams;
	auxParams.configMap = cm;
	auto *graphHandle = new TGraphBuilder(graphPath, {}, auxParams);

	Executor executor(cm, false);

	auto* assembly = new ColouringAssembly<TAlgo, TGraphBuilder>(*graphHandle);
	executor.registerAssembly("t", assembly);
	executor.executeAssembly("t");

	ASSERT_TRUE(assembly->algorithmSucceeded);
	ASSERT_TRUE(assembly->validationSucceeded);

	delete graphHandle;
}

TEST(ColouringMPAsync, FindsCorrectSolutionForSTG) {
	executeTest<GH, GraphColouringMPAsync>("resources/test/SimpleTestGraph.adjl");
}

TEST(ColouringMPAsync, FindsCorrectSolutionForComplete50) {
	executeTest<GH, GraphColouringMPAsync>("resources/test/complete50.adjl");
}

TEST(ColouringMP, FindsCorrectSolutionForSTG) {
	executeTest<GH, GraphColouringMp>("resources/test/SimpleTestGraph.adjl");
}

TEST(ColouringMP, FindsCorrectSolutionForComplete50) {
	executeTest<GH, GraphColouringMp>("resources/test/complete50.adjl");
}
