//
// Created by blueeyedhush on 14.07.17.
//

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <mpi.h>
#include <validators/BfsValidator.h>
#include <representations/ArrayBackedChunkedPartition.h>
#include <utils/TestUtils.h>

typedef int TestLocalId;
typedef int TestNumId;
using G = ArrayBackedChunkedPartition<TestLocalId, TestNumId>;
using ABCPGid = typename G::GidType;

TEST(BfsValidator, AcceptsCorrectSolutionForSTG) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	LOG(INFO) << "Initialized MPI";

	ABCPGraphBuilder<TestLocalId, TestNumId>  builder(size, rank);
	auto* gp = builder.buildGraph("resources/test/SimpleTestGraph.adjl", {0});
	auto bfsRoot = builder.getConvertedVertices()[0];
	LOG(INFO) << "Loaded graph from file";
	std::pair<ABCPGid*, int*> ps = bfsSolutionAsGids<TestLocalId, ABCPGid>("resources/test/STG.bfssol", size, rank);
	LOG(INFO) << "Loaded solution from file";

	BfsValidator<G> v(bfsRoot);
	bool validationResult = v.validate(gp, &ps);
	LOG(INFO) << "Executed validator";
	builder.destroyGraph(gp);
	delete[] ps.first;
	delete[] ps.second;

	ASSERT_TRUE(validationResult);
}

TEST(BfsValidator, RejectsIncorrectSolutionForSTG) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	LOG(INFO) << "Initialized MPI";

	ABCPGraphBuilder<TestLocalId, TestNumId> builder(size, rank);
	auto gp = builder.buildGraph("resources/test/SimpleTestGraph.adjl", {0});
	auto bfsRoot = builder.getConvertedVertices()[0];
	LOG(INFO) << "Loaded graph from file";
	std::pair<ABCPGid*, int*> ps = bfsSolutionAsGids<TestLocalId, ABCPGid>("resources/test/STG_incorrect.bfssol", size, rank);
	LOG(INFO) << "Loaded solution from file";

	BfsValidator<G> v(bfsRoot);
	bool validationResult = v.validate(gp, &ps);
	LOG(INFO) << "Executed validator";
	builder.destroyGraph(gp);
	delete[] ps.first;
	delete[] ps.second;

	ASSERT_FALSE(validationResult);
}
