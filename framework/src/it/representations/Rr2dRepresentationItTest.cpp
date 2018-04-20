//
// Created by blueeyedhush on 14.07.17.
//

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <mpi.h>
#include <representations/RoundRobin2DPartition.h>
#include <utils/TestUtils.h>

typedef int TestLocalId;
typedef int TestNumId;
using G = RoundRobin2DPartition<TestLocalId, TestNumId>;
using ABCPGid = typename G::GidType;

TEST(Rr2dRepresentation, PartitioningPreservesGraphStructure) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	LOG(INFO) << "Initialized MPI";

	ABCGraphHandle<TestLocalId, TestNumId>  builder("resources/test/SimpleTestGraph.adjl", size, rank, {0, 1, 2, 3});
	auto& gp = builder.getGraph();
	auto convertedVertices = builder.getConvertedVertices();
	LOG(INFO) << "Loaded graph from file";

	/* iterate masters & shadows, (source, target) pairs to file */

	MPI_Barrier(MPI_COMM_WORLD);
	/* load file, convert ids back to original */

	/* load expected edge list */

	/* compare against */
}
