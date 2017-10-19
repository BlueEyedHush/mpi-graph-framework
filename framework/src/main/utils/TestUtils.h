//
// Created by blueeyedhush on 03.07.17.
//

#ifndef FRAMEWORK_TESTUTILS_H
#define FRAMEWORK_TESTUTILS_H

#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <glog/logging.h>
#include <boost/numeric/conversion/cast.hpp>
#include <Prerequisites.h>
#include <GraphPartition.h>
#include <utils/CsvReader.h>
#include <utils/IndexPartitioner.h>
#include <representations/ArrayBackedChunkedPartition.h>

namespace details {
	using namespace IndexPartitioner;

	template<typename T>
	T* vecSubset(const std::vector<T> vec, const std::pair<int,int> range) {
		assert(vec.size() <= range.second);
		assert(range.first >= 0);
		assert(range.first < range.second);

		auto start = range.first;
		auto count = range.second-start;
		T* result = new T[count];
		for(int i = 0; i < count; i++) {
			result[i] = vec[start+i];
		}

		return result;
	}
}

/*
 * All pointers returned from helpers (those inside tuples) has to be deleted using delete[]
 */

template <typename TLocalId>
std::tuple<std::vector<NodeId>, std::vector<TLocalId>, std::vector<GraphDist>> bfsSolutionFromFile(std::string path) {
	CsvReader<OriginalVertexId> reader(path);

	auto distsFromFile = reader.getNextLine().value();
	std::vector<GraphDist> distances;
	std::transform(distsFromFile.begin(), distsFromFile.end(), std::back_inserter(distances), [](OriginalVertexId id) {
		return static_cast<GraphDist>(id);
	});

	std::vector<NodeId> nodeIds;
	std::vector<TLocalId> localIds;
	while(boost::optional<std::vector<OriginalVertexId>> line = reader.getNextLine()) {
		auto nodeId = boost::numeric_cast<NodeId>(line.value().at(0));
		auto localId = boost::numeric_cast<TLocalId>(line.value().at(1));
		nodeIds.push_back(nodeId);
		localIds.push_back(localId);
	}

	assert(nodeIds.size() == localIds.size());
	assert(localIds.size() == distances.size());

	return std::make_tuple(nodeIds, localIds, distances);
}

template <typename TSolutionType>
TSolutionType* loadPartialIntSolution(std::string solutionFilePath, int partitionCount, int partitionId) {
	CsvReader<TSolutionType> reader(solutionFilePath);
	std::vector<TSolutionType> solution = reader.getNextLine().value();
	auto range = details::get_range_for_partition(solution.size(), partitionCount, partitionId);
	return details::vecSubset(solution, range);
}

template <typename TLocalId>
std::tuple<NodeId*, TLocalId*, GraphDist*, size_t> loadBfsSolutionFromFile(std::string path,
                                                                           int partitionCount,
                                                                           int partitionId)
{
	auto t = bfsSolutionFromFile<TLocalId>(path);
	auto size = std::get<0>(t).size();
	auto range = details::get_range_for_partition(size, partitionCount, partitionId);
	auto N = details::vecSubset(std::get<0>(t), range);
	auto L = details::vecSubset(std::get<1>(t), range);
	auto D = details::vecSubset(std::get<2>(t), range);
	return std::make_tuple(N, L, D, size);
}

template <typename TLocalId, typename TGlobalId>
std::pair<TGlobalId*, int*> bfsSolutionAsGids(std::string path, int partitionCount, int partitionId) {
	NodeId* nIds;
	TLocalId* lIds;
	GraphDist* dists;
	size_t size;

	std::tie(nIds, lIds, dists, size) = loadBfsSolutionFromFile<TLocalId>(path, partitionCount, partitionId);

	TGlobalId *arr = new TGlobalId[size];
	for(size_t i = 0; i < size; i++) {
		arr[i] = TGlobalId(nIds[i], lIds[i]);
	}

	delete[] nIds;
	delete[] lIds;

	return std::make_pair(arr, dists);
}

#endif //FRAMEWORK_TESTUTILS_H
