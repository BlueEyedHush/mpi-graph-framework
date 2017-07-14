
#include <gtest/gtest.h>
#include <utils/TestUtils.h>

TEST(TestUtils, LoadPartialSolution) {
	int *ps = loadPartialIntSolution(std::string("resources/test/STG.csol"), 2, 1);

	ASSERT_EQ(ps[0], 1);
	ASSERT_EQ(ps[1], 0);

	delete[] ps;
}

TEST(TestUtils, LoadGidsFromFile) {
	auto gids = loadGidsFromFile("resources/test/gids");

	GlobalVertexId gid0;
	gid0.nodeId = 0;
	gid0.localId = 0;

	GlobalVertexId gid1;
	gid1.nodeId = 0;
	gid1.localId = 1;

	GlobalVertexId gid2;
	gid2.nodeId = 1;
	gid2.localId = 0;

	GlobalVertexId gid3;
	gid3.nodeId = 1;
	gid3.localId = 1;

	ASSERT_EQ(gids.size(), 4);
	ASSERT_EQ(gids[0], gid0);
	ASSERT_EQ(gids[1], gid1);
	ASSERT_EQ(gids[2], gid2);
	ASSERT_EQ(gids[3], gid3);
}