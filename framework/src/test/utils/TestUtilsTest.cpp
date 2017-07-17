
#include <gtest/gtest.h>
#include <utils/TestUtils.h>

TEST(TestUtils, GetPartition) {
	std::vector<int> solution = {5,6,7,8};

	auto p0 = getPartition(solution, 3, 0);
	auto p1 = getPartition(solution, 3, 1);
	auto p2 = getPartition(solution, 3, 2);

	ASSERT_EQ(p0[0], 5);
	ASSERT_EQ(p0[1], 6);
	ASSERT_EQ(p1[0], 7);
	ASSERT_EQ(p2[0], 8);
}

TEST(TestUtils, LoadPartialSolution) {
	int *ps = loadPartialIntSolution(std::string("resources/test/STG.csol"), 2, 1);

	ASSERT_EQ(ps[0], 1);
	ASSERT_EQ(ps[1], 0);

	delete[] ps;
}

TEST(TestUtils, LoadGidsFromFile) {
	auto gids = bspSolutionFromFile("resources/test/gids");

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
	ASSERT_EQ(gids[0], std::make_pair(gid0, 0));
	ASSERT_EQ(gids[1], std::make_pair(gid1, 1));
	ASSERT_EQ(gids[2], std::make_pair(gid2, 1));
	ASSERT_EQ(gids[3], std::make_pair(gid3, 1));
}

TEST(TestUtils, LoadBspSolutionFromFile) {
	auto solutionP0 = loadBspSolutionFromFile("resources/test/test.bspsol", 3, 0);
	auto solutionP1 = loadBspSolutionFromFile("resources/test/test.bspsol", 3, 1);
	auto solutionP2 = loadBspSolutionFromFile("resources/test/test.bspsol", 3, 2);

	ASSERT_EQ(solutionP0[0], std::make_pair(GlobalVertexId(0,0), 0));
	ASSERT_EQ(solutionP0[1], std::make_pair(GlobalVertexId(0,1), 1));
	ASSERT_EQ(solutionP1[0], std::make_pair(GlobalVertexId(1,0), 1));
	ASSERT_EQ(solutionP2[0], std::make_pair(GlobalVertexId(2,0), 1));
}