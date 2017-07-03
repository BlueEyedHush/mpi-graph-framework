//
// Created by blueeyedhush on 03.07.17.
//

#include "TestUtils.h"
#include <utils/CsvReader.h>
#include <utils/IndexPartitioner.h>

int* loadPartialSolution(std::string solutionFilePath, int partitionCount, int partitionId) {
	CsvReader reader(solutionFilePath);
	std::vector<int>& solution = reader.getNextLine().get();
	int* base = &solution[0];

	int start = IndexPartitioner::get_range_for_partition(solution.size(), partitionCount, partitionId).first;

	return base + start;
}
