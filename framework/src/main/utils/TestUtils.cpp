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
	for(int i = 0; i < count; i++) {
		result[i] = wholeSolution[start+i];
	}

	return result;
}

int* loadPartialIntSolution(std::string solutionFilePath, int partitionCount, int partitionId) {
	CsvReader reader(solutionFilePath);
	std::vector<int> solution = reader.getNextLine().value();
	return getPartition(solution, partitionCount, partitionId);
}

std::pair<std::vector<GlobalVertexId>, std::vector<int>> bfsSolutionFromFile(std::string path) {
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

	return std::make_pair(predecessors, distances);
}

std::pair<GlobalVertexId*, int*> loadBfsSolutionFromFile(std::string path, int partitionCount, int partitionId) {
	auto solution = bfsSolutionFromFile(path);
	auto P = getPartition(solution.first, partitionCount, partitionId);
	auto D = getPartition(solution.second, partitionCount, partitionId);
	return std::make_pair(P, D);
}
