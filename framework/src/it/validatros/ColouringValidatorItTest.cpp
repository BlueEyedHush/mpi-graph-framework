//
// Created by blueeyedhush on 03.07.17.
//

#include <gtest/gtest.h>
#include <mpi.h>
#include <validators/ColouringValidator.h>
#include <representations/ArrayBackedChunkedPartition.h>
#include <utils/TestUtils.h>
#include <Runner.h>

static void executeTest(std::string graphPath, std::string solutionPath, bool expectedValidationOutcome) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	using GB = ABCGraphHandle<int,int>;
	GB builder(graphPath, size, rank, {});
	auto gp = builder.getGraph();
	int *ps = loadPartialIntSolution<int>(solutionPath, size, rank);

	ColouringValidator<GB::GPType> v;
	bool validationResult = v.validate(&gp, ps);

	ASSERT_EQ(validationResult, expectedValidationOutcome);
}

TEST(ColouringValidator, AcceptsCorrectSolutionForSTG) {
	executeTest("resources/test/SimpleTestGraph.adjl", "resources/test/STG.csol", true);
}

TEST(ColouringValidator, RejectsIncorrectSolutionForSTG) {
	executeTest("resources/test/SimpleTestGraph.adjl", "resources/test/STG_incorrect.csol", false);
}

TEST(ColouringValidator, AcceptsCorrectSolutionForC50) {
	executeTest("resources/test/complete50.adjl", "resources/test/C50.csol", true);
}
