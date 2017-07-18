//
// Created by blueeyedhush on 14.07.17.
//

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <mpi.h>
#include <validators/BspValidator.h>
#include <representations/ArrayBackedChunkedPartition.h>
#include <utils/TestUtils.h>

TEST(BspValidator, AcceptsCorrectSolutionForSTG) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	LOG(INFO) << "Initialized MPI";

	auto gp = ArrayBackedChunkedPartition::fromFile("resources/test/SimpleTestGraph.adjl", size, rank);
	LOG(INFO) << "Loaded graph from file";
	std::pair<GlobalVertexId, int> *ps = loadBspSolutionFromFile("resources/test/STG.bspsol", size, rank);
	LOG(INFO) << "Loaded solution from file";

	BspValidator v;
	bool validationResult = v.validate(&gp, ps);
	LOG(INFO) << "Executed validator";

	ASSERT_TRUE(validationResult);
}

TEST(BspValidator, RejectsIncorrectSolutionForSTG) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	LOG(INFO) << "Initialized MPI";

	auto gp = ArrayBackedChunkedPartition::fromFile("resources/test/SimpleTestGraph.adjl", size, rank);
	LOG(INFO) << "Loaded graph from file";
	std::pair<GlobalVertexId, int> *ps = loadBspSolutionFromFile("resources/test/STG_incorrect.bspsol", size, rank);
	LOG(INFO) << "Loaded solution from file";

	BspValidator v;
	bool validationResult = v.validate(&gp, ps);
	LOG(INFO) << "Executed validator";

	ASSERT_FALSE(validationResult);
}
