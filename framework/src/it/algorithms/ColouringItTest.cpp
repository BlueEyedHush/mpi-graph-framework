
#include <gtest/gtest.h>
#include <mpi.h>
#include <utils/TestUtils.h>
#include <Runner.h>
#include <representations/AdjacencyListHashPartition.h>
#include <algorithms/GraphColouring.h>
#include <algorithms/GraphColouringAsync.h>
#include <validators/ColouringValidator.h>

void executeColouringTest(std::string graphPath,
                          Algorithm<int *> *algorithm,
                          Validator<int *> *validator) {
	GraphBuilder *graphBuilder = new ALHPGraphBuilder();
	GraphPartition *g = graphBuilder->buildGraph(graphPath);
	delete graphBuilder;

	AlgorithmExecutionResult r = runAndCheck<int *>(g,
	                                                [algorithm]() { return algorithm; },
	                                                [validator]() { return validator; });

	ASSERT_TRUE(r.algorithmStatus);
	ASSERT_TRUE(r.validatorStatus);
}


TEST(ColouringMPAsync, FindsCorrectSolutionForSTG) {
	auto a = new GraphColouringMPAsync();
	auto v = new ColouringValidator();

	executeColouringTest("resources/test/SimpleTestGraph.adjl", a, v);

	delete a;
	delete v;
}

TEST(ColouringMPAsync, FindsCorrectSolutionForComplete50) {
	auto a = new GraphColouringMPAsync();
	auto v = new ColouringValidator();

	executeColouringTest("resources/test/complete50.adjl", a, v);

	delete a;
	delete v;
}

TEST(ColouringMP, FindsCorrectSolutionForSTG) {
	auto a = new GraphColouringMP();
	auto v = new ColouringValidator();

	executeColouringTest("resources/test/SimpleTestGraph.adjl", a, v);

	delete a;
	delete v;
}

TEST(ColouringMP, FindsCorrectSolutionForComplete50) {
	auto a = new GraphColouringMP();
	auto v = new ColouringValidator();

	executeColouringTest("resources/test/complete50.adjl", a, v);

	delete a;
	delete v;
}