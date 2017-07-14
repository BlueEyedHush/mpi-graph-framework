//
// Created by blueeyedhush on 03.07.17.
//

#include "TestUtils.h"
#include <cstring>
#include <glog/logging.h>
#include <utils/CsvReader.h>
#include <utils/IndexPartitioner.h>

template<typename T>
T* getPartition(std::vector<T> wholeSolution, int partitionCount, int partitionId) {
	// @ToDo type conflict
	auto p = IndexPartitioner::get_range_for_partition(wholeSolution.size(), partitionCount, partitionId);
	int start = p.first;
	int end = p.second;
	int count = end-start;

	T* result = new T[count];
	memcpy(result, wholeSolution.data() + start, count*sizeof(int));

	return result;
}

int* loadPartialIntSolution(std::string solutionFilePath, int partitionCount, int partitionId) {
	CsvReader reader(solutionFilePath);
	std::vector<int> solution = reader.getNextLine().value();
	return getPartition(solution, partitionCount, partitionId);
}

std::vector<std::pair<GlobalVertexId, int>> bspSolutionFromFile(std::string path) {
	CsvReader reader(path);

	std::vector<int> distances = reader.getNextLine().value();

	std::vector<GlobalVertexId> predecessors;
	while(boost::optional<std::vector<int>> line = reader.getNextLine()) {
		GlobalVertexId id;
		id.nodeId = line.value().at(0);
		id.localId = line.value().at(1);
		predecessors.push_back(id);
	}

	LOG_IF(FATAL, (predecessors.size() != distances.size()))
		<< "predecessors and distances have different sizes"
        << "(" << predecessors.size() << " vs. " << distances.size() << ")";

	std::vector<std::pair<GlobalVertexId, int>> zipped;
	for(size_t i = 0; i < predecessors.size(); i++) {
		zipped.push_back(std::make_pair(predecessors.at(i), distances.at(i)));
	}

	return zipped;
}

std::pair<GlobalVertexId, int> *loadBspSolutionFromFile(std::string path, int partitionCount, int partitionId) {
	return getPartition(bspSolutionFromFile(path), partitionCount, partitionId);
}
