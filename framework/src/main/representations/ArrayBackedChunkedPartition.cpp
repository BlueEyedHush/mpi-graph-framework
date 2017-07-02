//
// Created by blueeyedhush on 06.05.17.
//

#include "ArrayBackedChunkedPartition.h"
#include <algorithm>
#include <climits>
#include <utils/IndexPartitioner.h>



ArrayBackedChunkedPartition::ArrayBackedChunkedPartition(int _E, int _V, int _P, int _partitionId, int *_adjacencyList, int *_offsets) :
	E(_E), V(_V), P(_P), partitionId(_partitionId), adjacencyList(_adjacencyList), offsets(_offsets)
{
	auto v_range = IndexPartitioner::get_range_for_partition(V, P, partitionId);
	first = v_range.first;
	one_after_last = v_range.second;
}

int ArrayBackedChunkedPartition::getLocalVertexCount() {
	return one_after_last - first;
}

void ArrayBackedChunkedPartition::forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) {
	int *neighbours = adjacencyList + offsets[first + id];
	int count = (id != V - 1) ? (offsets[id+1] - offsets[id]) : (E - offsets[V-1]);

	GlobalVertexId fakeGlobalId;
	for(int i = 0; i < count; i++) {
		LocalVertexId v_id = neighbours[i];
		fakeGlobalId.nodeId = IndexPartitioner::get_partition_from_index(V, P, v_id);
		int start = IndexPartitioner::get_range_for_partition(V, P, fakeGlobalId.nodeId).first;
		fakeGlobalId.localId = v_id - start;
		f(fakeGlobalId);
	}
}

bool ArrayBackedChunkedPartition::isLocalVertex(GlobalVertexId id) {
	return id.nodeId == partitionId;
};

void ArrayBackedChunkedPartition::forEachLocalVertex(std::function<void(LocalVertexId)> f) {
	for(int i = 0; i < one_after_last - first; i++) {
		f(i);
	}
}

NodeId ArrayBackedChunkedPartition::getNodeId() {
	return partitionId;
}

unsigned long long ArrayBackedChunkedPartition::toNumerical(GlobalVertexId id) {
	unsigned int halfBitsInUll = (sizeof(unsigned long long)*CHAR_BIT)/2;
	unsigned long long numerical = ((unsigned long long) id.localId) << halfBitsInUll;
	numerical |= ((unsigned int) id.nodeId);
	return numerical;
}
