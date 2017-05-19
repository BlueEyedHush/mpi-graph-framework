//
// Created by blueeyedhush on 06.05.17.
//

#include <mpi.h>
#include <algorithm>
#include <climits>
#include "SimpleStaticGraph.h"

int E[E_N] = {
	1, 2, 3,
	0, 2, 3,
	0, 1, 3,
	0, 1, 2
};

int V_OFFSETS[V_N] = {
	0,
	3,
	6,
	9
};

SimpleStaticGraph::SimpleStaticGraph() {
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	base_allocation = V_N/world_size;
	excess = V_N % world_size;

	std::pair <int, int> v_range = get_process_range(world_rank);
	first = v_range.first;
	one_after_last = v_range.second;
}

int SimpleStaticGraph::getLocalVertexCount() {
	return one_after_last - first;
}

void SimpleStaticGraph::forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) {
	int *neighbours = &E[V_OFFSETS[first + id]];
	int count = (id != V_N - 1) ? (V_OFFSETS[id+1] - V_OFFSETS[id]) : (E_N - V_OFFSETS[V_N-1]);

	GlobalVertexId fakeGlobalId;
	for(int i = 0; i < count; i++) {
		LocalVertexId v_id = neighbours[i];
		fakeGlobalId.nodeId = get_node_id_for(V_N, world_size, world_rank, v_id);
		int start = base_allocation * fakeGlobalId.nodeId + std::min(fakeGlobalId.nodeId, excess);
		fakeGlobalId.localId = v_id - start;
		f(fakeGlobalId);
	}
}

bool SimpleStaticGraph::isLocalVertex(GlobalVertexId id) {
	return id.nodeId == world_rank;
};

void SimpleStaticGraph::forEachLocalVertex(std::function<void(LocalVertexId)> f) {
	for(int i = 0; i < one_after_last - first; i++) {
		f(i);
	}
}

NodeId SimpleStaticGraph::getNodeId() {
	return world_rank;
}

unsigned long long SimpleStaticGraph::toNumerical(GlobalVertexId id) {
	unsigned int halfBitsInUll = (sizeof(unsigned long long)*CHAR_BIT)/2;
	unsigned long long numerical = ((unsigned long long) id.localId) << halfBitsInUll;
	numerical |= ((unsigned int) id.nodeId);
	return numerical;
}


/**
 * Each node gets vertex_no/world_size vertices. The excess (k = vertex_no % world_rank) is distributed between
 * first k nodes
 *
 * @param rank - rank of the requesting node
 * @return std::pair where first is index of first vertex and second is index of one vertex after last
 */
std::pair <int, int> SimpleStaticGraph::get_process_range(int rank) {
	int count = base_allocation + std::max(0, std::min(1, excess - rank));
	int start = base_allocation * rank + std::min(rank, excess);

	return std::make_pair(start, start+count);
}

int SimpleStaticGraph::get_node_id_for(int V, int N, int current_rank, int id) {
	int base_allocation = V/N;
	int excess = V%N;
	int prognosed_node = id/base_allocation;
	/* above would be target node if we didn't decided to partition excess the way we did
	 * excess must be smaller than bucket width, so our vertex won't go much further than node_id back */
	int prognosed_start = base_allocation * prognosed_node + std::min(prognosed_node, excess);

	int actual_node_id = (id >= prognosed_start) ? prognosed_node : prognosed_node-1;
}



