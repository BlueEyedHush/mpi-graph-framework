
#include <gtest/gtest.h>
#include <utils/TestUtils.h>

TEST(TestUtils, vecSubsetPreconditions) {
	std::vector<int> solution = {5,6,7,8};

	EXPECT_DEATH((details::vecSubset<int>(solution, std::make_pair(2, 2))), "Debug Assertion failed");
	EXPECT_DEATH((details::vecSubset<int>(solution, std::make_pair(6, 7))), "Debug Assertion failed");
	EXPECT_DEATH((details::vecSubset<int>(solution, std::make_pair(-1, 0))), "Debug Assertion failed");
}

TEST(TestUtils, vecSubsetResultCorrectness) {
	std::vector<int> solution = {5,6,7,8};

	auto p0 = details::vecSubset(solution, std::make_pair(0,2));
	auto p1 = details::vecSubset(solution, std::make_pair(1,3));

	ASSERT_EQ(p0[0], 5);
	ASSERT_EQ(p0[1], 6);
	ASSERT_EQ(p1[0], 6);
	ASSERT_EQ(p1[1], 7);
	
	delete[] p0;
	delete[] p1;
}

TEST(TestUtils, LoadPartialSolution) {
	int *ps = loadPartialIntSolution<int>(std::string("resources/test/STG.csol"), 2, 1);

	ASSERT_EQ(ps[0], 1);
	ASSERT_EQ(ps[1], 0);

	delete[] ps;
}

TEST(TestUtils, LoadFullBfsSolutionFromFileSizes) {
	std::vector<NodeId> n;
	std::vector<unsigned int> lid;
	std::vector<GraphDist> dist;
	std::tie(n, lid, dist) = bfsSolutionFromFile<unsigned int>("resources/test/gids");
	
	ASSERT_EQ(n.size(), 4);
	ASSERT_EQ(lid.size(), 4);
	ASSERT_EQ(dist.size(), 4);
}

TEST(TestUtils, LoadGidsFromFile) {
	std::vector<NodeId> n;
	std::vector<unsigned int> lid;
	std::vector<GraphDist> dist;
	std::tie(n, lid, dist) = bfsSolutionFromFile<unsigned int>("resources/test/gids");

	ASSERT_EQ(n[0], 0);
	ASSERT_EQ(n[1], 0);
	ASSERT_EQ(n[2], 1);
	ASSERT_EQ(n[3], 1);
	ASSERT_EQ(lid[0], 0);
	ASSERT_EQ(lid[1], 1);
	ASSERT_EQ(lid[2], 0);
	ASSERT_EQ(lid[3], 1);
}

TEST(TestUtils, LoadDistancesFromFile) {
	std::vector<NodeId> n;
	std::vector<unsigned int> lid;
	std::vector<GraphDist> dist;
	std::tie(n, lid, dist) = bfsSolutionFromFile<unsigned int>("resources/test/gids");

	ASSERT_EQ(dist[0], 0);
	ASSERT_EQ(dist[1], 1);
	ASSERT_EQ(dist[2], 1);
	ASSERT_EQ(dist[3], 1);
}

TEST(TestUtils, LoadBfsSolutionFromFile) {
	NodeId *n0, *n1, *n2;
	unsigned int *lid0, *lid1, *lid2;
	GraphDist *dist0, *dist1, *dist2;

	std::tie(n0, lid0, dist0, std::ignore) = loadBfsSolutionFromFile<unsigned int>("resources/test/test.bfssol", 3, 0);
	std::tie(n1, lid1, dist1, std::ignore) = loadBfsSolutionFromFile<unsigned int>("resources/test/test.bfssol", 3, 1);
	std::tie(n2, lid2, dist2, std::ignore) = loadBfsSolutionFromFile<unsigned int>("resources/test/test.bfssol", 3, 2);

	ASSERT_EQ(n0[0], 0); ASSERT_EQ(lid0[0], 0); ASSERT_EQ(dist0[0], 0);
	ASSERT_EQ(n0[1], 0); ASSERT_EQ(lid0[1], 1); ASSERT_EQ(dist0[1], 1);
	ASSERT_EQ(n1[0], 1); ASSERT_EQ(lid0[0], 0); ASSERT_EQ(dist1[0], 1);
	ASSERT_EQ(n2[0], 2); ASSERT_EQ(lid0[0], 0); ASSERT_EQ(dist2[0], 1);

	delete[] n0;
	delete[] lid0;
	delete[] dist0;
	delete[] n1;
	delete[] lid1;
	delete[] dist1;
	delete[] n2;
	delete[] lid2;
	delete[] dist2;
}