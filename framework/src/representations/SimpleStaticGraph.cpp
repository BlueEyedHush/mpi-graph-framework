//
// Created by blueeyedhush on 06.05.17.
//

#include <mpi.h>
#include <algorithm>
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

int SimpleStaticGraph::getVertexCount() {
	return V_N;
}

int SimpleStaticGraph::getLocalVertexCount() {
	return one_after_last - first;
}

int SimpleStaticGraph::getEdgeCount() {
	return E_N;
}

void SimpleStaticGraph::forEachNeighbour(int v, std::function<void(int)> f) {
	int *neighbours = &E[V_OFFSETS[v]];
	int count = (v != V_N - 1) ? (V_OFFSETS[v+1] - V_OFFSETS[v]) : (E_N - V_OFFSETS[V_N-1]);

	for(int i = 0; i < count; i++) {
		f(neighbours[i]);
	}
}

void SimpleStaticGraph::forEachLocalVertex(std::function<void(int)> f) {
	for(int i = first; i < one_after_last; i++) {
		f(i);
	}
}

int SimpleStaticGraph::getNodeResponsibleForVertex(int id) {
	int prognosed_node = id/base_allocation;
	/* above would be target node if we didn't decided to partition excess the way we did
	 * excess must be smaller than bucket width, so our vertex won't go much further than node_id back */
	int prognosed_start = base_allocation * prognosed_node + std::min(prognosed_node, excess);

	return (id >= prognosed_start) ? prognosed_node : prognosed_node-1;
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
};