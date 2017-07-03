
#include <gtest/gtest.h>
#include <glog/logging.h>
#include <mpi.h>

int main(int argc, char* argv[]) {
	::testing::InitGoogleTest(&argc, argv);
	google::InitGoogleLogging(argv[0]);
	MPI_Init(&argc, &argv);
	FLAGS_logtostderr = true;
	int result = RUN_ALL_TESTS();
	MPI_Finalize();
	return result;
}