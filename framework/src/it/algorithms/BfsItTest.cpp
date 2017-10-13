
#include <gtest/gtest.h>
#include <mpi.h>
#include <utils/TestUtils.h>
#include <Runner.h>
#include <representations/AdjacencyListHashPartition.h>
#include <algorithms/Bfs.h>
#include <validators/BfsValidator.h>

void executeColouringTest(std::string graphPath,
                          Algorithm<std::pair<GlobalVertexId*, int*>*> *algorithm,
                          Validator<std::pair<GlobalVertexId*, int*>*> *validator) {
	GraphBuilder *graphBuilder = new ALHPGraphBuilder();
	GraphPartition *g = graphBuilder->buildGraph(graphPath);
	delete graphBuilder;

	AlgorithmExecutionResult r = runAndCheck<std::pair<GlobalVertexId*, int*>*>(g, 
	                                                                            [algorithm]() { return algorithm; }, 
	                                                                            [validator]() { return validator; });

	graphBuilder->destroyGraph(g);
	ASSERT_TRUE(r.algorithmStatus);
	ASSERT_TRUE(r.validatorStatus);
}


TEST(Bfs_Mp_FixedMsgLen_1D_2CommRounds, FindsCorrectSolutionForSTG) {
	GlobalVertexId bfsRoot(0, 0);
	auto a = new Bfs_Mp_FixedMsgLen_1D_2CommRounds(bfsRoot);
	auto v = new BfsValidator(bfsRoot);

	executeColouringTest("resources/test/SimpleTestGraph.adjl", a, v);

	delete a;
	delete v;
}

TEST(Bfs_Mp_FixedMsgLen_1D_2CommRounds, FindsCorrectSolutionForComplete50) {
	GlobalVertexId bfsRoot(0, 0);
	auto a = new Bfs_Mp_FixedMsgLen_1D_2CommRounds(bfsRoot);
	auto v = new BfsValidator(bfsRoot);

	executeColouringTest("resources/test/complete50.adjl", a, v);

	delete a;
	delete v;
}

TEST(Bfs_Mp_VarMsgLen_1D_2CommRounds, FindsCorrectSolutionForSTG) {
	GlobalVertexId bfsRoot(0, 0);
	auto a = new Bfs_Mp_VarMsgLen_1D_2CommRounds(bfsRoot);
	auto v = new BfsValidator(bfsRoot);

	executeColouringTest("resources/test/SimpleTestGraph.adjl", a, v);

	delete a;
	delete v;
}

TEST(Bfs_Mp_VarMsgLen_1D_2CommRounds, FindsCorrectSolutionForComplete50) {
	GlobalVertexId bfsRoot(0, 0);
	auto a = new Bfs_Mp_VarMsgLen_1D_2CommRounds(bfsRoot);
	auto v = new BfsValidator(bfsRoot);

	executeColouringTest("resources/test/complete50.adjl", a, v);

	delete a;
	delete v;
}

TEST(Bfs_Mp_VarMsgLen_1D_1CommsTag, FindsCorrectSolutionForSTG) {
	GlobalVertexId bfsRoot(0, 0);
	auto a = new Bfs_Mp_VarMsgLen_1D_1CommsTag(bfsRoot);
	auto v = new BfsValidator(bfsRoot);

	executeColouringTest("resources/test/SimpleTestGraph.adjl", a, v);

	delete a;
	delete v;
}

TEST(Bfs_Mp_VarMsgLen_1D_1CommsTag, FindsCorrectSolutionForComplete50) {
	GlobalVertexId bfsRoot(0, 0);
	auto a = new Bfs_Mp_VarMsgLen_1D_1CommsTag(bfsRoot);
	auto v = new BfsValidator(bfsRoot);

	executeColouringTest("resources/test/complete50.adjl", a, v);

	delete a;
	delete v;
}