//
// Created by blueeyedhush on 06.05.17.
//

#include "ArrayBackedChunkedPartition.h"
#include <algorithm>
#include <climits>
#include <utils/IndexPartitioner.h>
#include <utils/AdjacencyListReader.h>

ABCPGraphBuilder::ABCPGraphBuilder(int partitionCount, int partitionId)
		: P(partitionCount), partitionId(partitionId) {}

GraphPartition* ABCPGraphBuilder::buildGraph(std::string path) {
	AdjacencyListReader reader(path);
	int *adjacencyList = new int[reader.getEdgeCount()];
	int *offsets = new int[reader.getVertexCount()];

	int nextOffset = 0;
	while(boost::optional<VertexSpec> oVertexSpec = reader.getNextVertex()) {
		auto spec = *oVertexSpec;
		offsets[spec.vertexId] = nextOffset;

		for(auto neighId: spec.neighbours) {
			adjacencyList[nextOffset] = neighId;
			nextOffset += 1;
		}
	}

	return new ArrayBackedChunkedPartition(
			reader.getEdgeCount(),
			reader.getVertexCount(),
			P,
			partitionId,
			adjacencyList,
			offsets);
}

void ABCPGraphBuilder::destroyGraph(const GraphPartition *p) {
	delete p;
}


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
}

void ArrayBackedChunkedPartition::forEachLocalVertex(std::function<void(LocalVertexId)> f) {
	for(int i = 0; i < one_after_last - first; i++) {
		f(i);
	}
}

NodeId ArrayBackedChunkedPartition::getNodeId() {
	return partitionId;
}

unsigned long long ArrayBackedChunkedPartition::toNumerical(GlobalVertexId id) {
	int partitionOffset = IndexPartitioner::get_range_for_partition(V, P, id.nodeId).first;
	// @ToDo: type collision
	return partitionOffset + id.localId;
}

int ArrayBackedChunkedPartition::getMaxLocalVertexCount() {
	int excess = (V % P == 0) ? 0 : 1;
	return V/P + excess;
}
