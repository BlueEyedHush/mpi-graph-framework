//
// Created by blueeyedhush on 03.07.17.
//

#include <gtest/gtest.h>
#include <mpi.h>
#include <validators/ColouringValidator.h>
#include <representations/ArrayBackedChunkedPartition.h>
#include <utils/TestUtils.h>

TEST(ColouringValidator, AcceptsCorrectSolutionForSTG) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	auto gp = ArrayBackedChunkedPartition::fromFile(std::string("resources/test/SimpleTestGraph.adjl"), size, rank);
	int *ps = loadPartialIntSolution(std::string("resources/test/STG.csol"), size, rank);

	ColouringValidator v;
	bool validationResult = v.validate(&gp, ps);

	ASSERT_TRUE(validationResult);
}

TEST(ColouringValidator, AcceptsCorrectSolutionForC50) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	auto gp = ArrayBackedChunkedPartition::fromFile(std::string("resources/test/complete50.adjl"), size, rank);
	int *ps = loadPartialIntSolution(std::string("resources/test/C50.csol"), size, rank);

	ColouringValidator v;
	bool validationResult = v.validate(&gp, ps);

	ASSERT_TRUE(validationResult);
}