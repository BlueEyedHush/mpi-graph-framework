
#include <gtest/gtest.h>
#include <utils/TestUtils.h>

TEST(TestUtils, LoadPartialSolution) {
	int *ps = loadPartialSolution(std::string("resources/test/STG.csol"), 2, 1);

	ASSERT_EQ(ps[0], 1);
	ASSERT_EQ(ps[1], 0);

	delete[] ps;
}