
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
	auto gids = bspSolutionFromFile("resources/test/gids").first;

	ASSERT_EQ(gids.size(), 4);
	ASSERT_EQ(gids[0], GlobalVertexId(0, 0));
	ASSERT_EQ(gids[1], GlobalVertexId(0, 1));
	ASSERT_EQ(gids[2], GlobalVertexId(1, 0));
	ASSERT_EQ(gids[3], GlobalVertexId(1, 1));
}

TEST(TestUtils, LoadDistancesFromFile) {
	auto dists = bspSolutionFromFile("resources/test/gids").second;

	ASSERT_EQ(dists.size(), 4);
	ASSERT_EQ(dists[0], 0);
	ASSERT_EQ(dists[1], 1);
	ASSERT_EQ(dists[2], 1);
	ASSERT_EQ(dists[3], 1);
}

TEST(TestUtils, LoadBspSolutionFromFile) {
	auto solutionP0 = loadBspSolutionFromFile("resources/test/test.bspsol", 3, 0);
	auto solutionP1 = loadBspSolutionFromFile("resources/test/test.bspsol", 3, 1);
	auto solutionP2 = loadBspSolutionFromFile("resources/test/test.bspsol", 3, 2);

	ASSERT_EQ(solutionP0.first[0], GlobalVertexId(0,0));
	ASSERT_EQ(solutionP0.first[1], GlobalVertexId(0,1));
	ASSERT_EQ(solutionP1.first[0], GlobalVertexId(1,0));
	ASSERT_EQ(solutionP2.first[0], GlobalVertexId(2,0));

	ASSERT_EQ(solutionP0.second[0], 0);
	ASSERT_EQ(solutionP0.second[1], 1);
	ASSERT_EQ(solutionP1.second[0], 1);
	ASSERT_EQ(solutionP2.second[0], 1);
}