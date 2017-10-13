//
// Created by blueeyedhush on 14.07.17.
//

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <mpi.h>
#include <validators/BfsValidator.h>
#include <representations/ArrayBackedChunkedPartition.h>
#include <utils/TestUtils.h>

TEST(BfsValidator, AcceptsCorrectSolutionForSTG) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	LOG(INFO) << "Initialized MPI";

	ABCPGraphBuilder builder(size, rank);
	auto gp = builder.buildGraph("resources/test/SimpleTestGraph.adjl");
	LOG(INFO) << "Loaded graph from file";
	std::pair<GlobalVertexId*, int*> ps = loadBfsSolutionFromFile("resources/test/STG.bfssol", size, rank);
	LOG(INFO) << "Loaded solution from file";

	BfsValidator v(GlobalVertexId(0,0));
	bool validationResult = v.validate(gp, &ps);
	LOG(INFO) << "Executed validator";
	builder.destroyGraph(gp);

	ASSERT_TRUE(validationResult);
}

TEST(BfsValidator, RejectsIncorrectSolutionForSTG) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	LOG(INFO) << "Initialized MPI";

	ABCPGraphBuilder builder(size, rank);
	auto gp = builder.buildGraph("resources/test/SimpleTestGraph.adjl");
	LOG(INFO) << "Loaded graph from file";
	std::pair<GlobalVertexId*, int*> ps = loadBfsSolutionFromFile("resources/test/STG_incorrect.bfssol", size, rank);
	LOG(INFO) << "Loaded solution from file";

	BfsValidator v(GlobalVertexId(0,0));
	bool validationResult = v.validate(gp, &ps);
	LOG(INFO) << "Executed validator";
	builder.destroyGraph(gp);

	ASSERT_FALSE(validationResult);
}
