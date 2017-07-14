//
// Created by blueeyedhush on 14.07.17.
//

#include <gtest/gtest.h>
#include <mpi.h>
#include <validators/BspValidator.h>
#include <representations/ArrayBackedChunkedPartition.h>
#include <utils/TestUtils.h>

TEST(BspValidator, AcceptsCorrectSolutionForSTG) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	auto gp = ArrayBackedChunkedPartition::fromFile("resources/test/SimpleTestGraph.adjl", size, rank);
	std::pair<GlobalVertexId, int> *ps = loadBspSolutionFromFile("resources/test/STG.bspsol", size, rank);

	BspValidator v;
	bool validationResult = v.validate(&gp, ps);

	ASSERT_TRUE(validationResult);
}
