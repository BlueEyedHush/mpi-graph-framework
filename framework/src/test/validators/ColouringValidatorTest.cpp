//
// Created by blueeyedhush on 03.07.17.
//

#include <gtest/gtest.h>
#include <utils/CsvReader.h>
#include <utils/IndexPartitioner.h>

int* loadPartialSolution(std::string solutionFilePath, int partitionCount, int partitionId) {
	CsvReader reader(solutionFilePath);
	std::vector<int>& solution = reader.getNextLine().get();
	int* base = &solution[0];

	int start = IndexPartitioner::get_range_for_partition(solution.size(), partitionCount, partitionId).first;

	return base + start;
}

TEST(ColouringValidator, LoadPartialSolution) {
	int *ps = loadPartialSolution(std::string("resources/test/STG.csol"), 2, 1);

	ASSERT_EQ(ps[0], 1);
	ASSERT_EQ(ps[1], 0);
}