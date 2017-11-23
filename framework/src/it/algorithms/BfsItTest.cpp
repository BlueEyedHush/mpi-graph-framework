
#include <gtest/gtest.h>
#include <utils/TestUtils.h>
#include <Assembly.h>
#include <representations/AdjacencyListHashPartition.h>
#include <algorithms/bfs/Bfs1CommsRound.h>
#include <algorithms/bfs/BfsFixedMessage.h>
#include <algorithms/bfs/BfsVarMessage.h>
#include <assemblies/BfsAssembly.h>

using GH = ALHGraphHandle<int, int>;

template <typename TGraphBuilder, template<typename> class TAlgo>
static void executeTest(std::string graphPath, ull originalRootId)
{
	auto *graphHandle = new TGraphBuilder(graphPath, {originalRootId});
	BfsAssembly<TAlgo, TGraphBuilder> assembly(*graphHandle);
	assembly.run();

	ASSERT_TRUE(assembly.algorithmSucceeded);
	ASSERT_TRUE(assembly.validationSucceeded);

	delete graphHandle;
}


TEST(Bfs_Mp_FixedMsgLen_1D_2CommRounds, FindsCorrectSolutionForSTG) {
	executeTest<GH, Bfs_Mp_FixedMsgLen_1D_2CommRounds>("resources/test/SimpleTestGraph.adjl", 0);
}

TEST(Bfs_Mp_FixedMsgLen_1D_2CommRounds, FindsCorrectSolutionForComplete50) {
	executeTest<GH, Bfs_Mp_FixedMsgLen_1D_2CommRounds>("resources/test/complete50.adjl", 0);
}

TEST(Bfs_Mp_VarMsgLen_1D_2CommRounds, FindsCorrectSolutionForSTG) {
	executeTest<GH, Bfs_Mp_VarMsgLen_1D_2CommRounds>("resources/test/SimpleTestGraph.adjl", 0);
}

TEST(Bfs_Mp_VarMsgLen_1D_2CommRounds, FindsCorrectSolutionForComplete50) {
	executeTest<GH, Bfs_Mp_VarMsgLen_1D_2CommRounds>("resources/test/complete50.adjl", 0);
}

TEST(Bfs_Mp_VarMsgLen_1D_1CommsTag, FindsCorrectSolutionForSTG) {
	executeTest<GH, Bfs_Mp_VarMsgLen_1D_1CommsTag>("resources/test/SimpleTestGraph.adjl", 0);
}

TEST(Bfs_Mp_VarMsgLen_1D_1CommsTag, FindsCorrectSolutionForComplete50) {
	executeTest<GH, Bfs_Mp_VarMsgLen_1D_1CommsTag>("resources/test/complete50.adjl", 0);
}