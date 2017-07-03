//
// Created by blueeyedhush on 03.07.17.
//

#include "TestUtils.h"
#include <cstring>
#include <utils/CsvReader.h>
#include <utils/IndexPartitioner.h>

int* loadPartialSolution(std::string solutionFilePath, int partitionCount, int partitionId) {
	CsvReader reader(solutionFilePath);
	std::vector<int> solution = reader.getNextLine().value();

	auto p = IndexPartitioner::get_range_for_partition(solution.size(), partitionCount, partitionId);
	int start = p.first;
	int end = p.second;
	int count = end-start;

	int* result = new int[count];
	memcpy(result, solution.data() + start, count*sizeof(int));

	return result;
}
