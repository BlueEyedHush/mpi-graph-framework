//
// Created by blueeyedhush on 03.07.17.
//

#include "TestUtils.h"
#include <cstring>
#include <utils/CsvReader.h>
#include <utils/IndexPartitioner.h>

int* loadPartialIntSolution(std::string solutionFilePath, int partitionCount, int partitionId) {
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

std::vector<GlobalVertexId> loadGidsFromFile(std::string path) {
	CsvReader reader(path);

	std::vector<GlobalVertexId> result;
	while(boost::optional<std::vector<int>> line = reader.getNextLine()) {
		GlobalVertexId id;
		id.nodeId = line.value().at(0);
		id.localId = line.value().at(1);
		result.push_back(id);
	}

	return result;
}
