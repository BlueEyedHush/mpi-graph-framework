
#include <gtest/gtest.h>
#include <mpi.h>

TEST(Misc, Example) {
	MPI_Barrier(MPI_COMM_WORLD);
	int rank = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	printf("\nExample integration test. Rank: %d\n", rank);
	MPI_Barrier(MPI_COMM_WORLD);
}