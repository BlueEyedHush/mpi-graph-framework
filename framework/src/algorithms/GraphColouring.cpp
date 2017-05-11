//
// Created by blueeyedhush on 06.05.17.
//

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <cstddef>
#include <utility>
#include <set>
#include <stack>
#include <list>
#include <mpi.h>
#include "GraphColouring.h"

/*
 * @ToDo:
 * - debug logging
 * - another list for 0ed vertices
 * - look at memory usage (valgrind, review types, use pointers for buffers and cleanup asap, free send buffers, free requests & receive buffers)
 * - don't create set for vertices with 0 wait_count
 * - single initialization loop
 * - C++ features: auto loops (instead iterators), use std iterator instead of your own
 * - unit tests
 * - optimize functions for vertex <-> node placement? + unit tests for them
 * - correct error handling
 * - try to replace cleanup loops with waitsome
 * - dynamically adjust number of outstanding requests
 * - more generic buffer pool class + extend for special request buffer pool
 * - same function for cleaning up receive & send pools
 */

#define MPI_TAG 0
#define OUTSTANDING_RECEIVE_REQUESTS 5

struct Message {
	int receiving_node_id;
	int used_colour;
};

void register_mpi_message(MPI_Datatype *memory) {
	int blocklengths[] = {1, 1};
	MPI_Aint displacements[] = {offsetof(Message, receiving_node_id), offsetof(Message, used_colour)};
	MPI_Datatype building_types[] = {MPI_INT, MPI_INT};
	MPI_Type_create_struct(2, blocklengths, displacements, building_types, memory);

	MPI_Type_commit(memory);
}

struct BufferAndRequest {
	MPI_Request request;
	Message buffer;
};

class BufferPool {
private:
	std::stack<BufferAndRequest*> freeBuffers;
	std::list<BufferAndRequest*> allocatedBuffers;

public:
	BufferPool() {}
	~BufferPool() {}

	BufferAndRequest *allocateBuffer() {
		BufferAndRequest *b = nullptr;
		if (freeBuffers.empty()) {
			b = new BufferAndRequest();
		} else {
			b = freeBuffers.top();
			freeBuffers.pop();
		}
		allocatedBuffers.push_front(b);
		return b;
	}

	void tryFreeBuffers(bool (*freer)(BufferAndRequest*)) {
		for (auto it = allocatedBuffers.begin(); it != allocatedBuffers.end();) {
			if (freer(*it)) {
				freeBuffers.push(*it);
				it = allocatedBuffers.erase(it);
			} else {
				it++;
			}
		}
	}
};

/**
 * Each node gets vertex_no/world_size vertices. The excess (k = vertex_no % world_rank) is distributed between
 * first k nodes
 *
 * @param V - number of vertices
 * @param N - number of nodes
 * @param rank - rank of the requesting node
 * @return std::pair where first is index of first vertex and second is index of one vertex after last
 */
std::pair <int, int> get_current_process_range(int V, int N, int rank) {
	int base = V/N;
	int excess = V%N;

	int count = base + std::max(0, std::min(1, excess - rank));
	int start = base * rank + std::min(rank, excess);

	return std::make_pair(count, start);
};

int get_node_from_vertex_id(int V, int N, int vertex_id) {
	int base = V/N;
	int excess = V%N;

	int prognosed_node = vertex_id/base;
	/* above would be target node if we didn't decided to partition excess the way we did
	 * excess must be smaller than bucket width, so our vertex won't go much further than node_id back */
	int prognosed_start = base * prognosed_node + std::min(prognosed_node, excess);

	return (vertex_id >= prognosed_start) ? prognosed_node : prognosed_node-1;
}

bool GraphColouring::run(Graph *g) {
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	std::pair <int, int> v_range = get_current_process_range(g->getVertexCount(), world_size, world_rank);
	int v_count = v_range.second - v_range.first;

	int *wait_counters = new int[v_count];
	for(int i = 0; i < v_count; i++) wait_counters[i] = 0;

	std::set<int> *used_colours = new std::set<int>[v_count]();

	MPI_Datatype mpi_message_type;
	register_mpi_message(&mpi_message_type);

	BufferPool sendBuffers;

	Message *receive_buffers = new Message[OUTSTANDING_RECEIVE_REQUESTS];
	MPI_Request* *receive_requests = new MPI_Request*[OUTSTANDING_RECEIVE_REQUESTS];

	/* start outstanding receive requests */
	for(int i = 0; i < OUTSTANDING_RECEIVE_REQUESTS; i++) {
		MPI_Request *receive_request = new MPI_Request;
		receive_requests[i] = receive_request;
		MPI_Irecv(&receive_buffers[i], 1, mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, receive_request);
	}

	/* gather information about rank of neighbours */

	for(int v_id = v_range.first; v_id < v_range.second; v_id++) {
		int id = 0;
		Iterator<int> *neighIt = g->getNeighbourIterator(world_rank);
		while(neighIt->hasNext()) {
			id = neighIt->next();
			if (id > world_rank) {
				wait_counters[v_id]++;
			}

			fprintf(stderr, "[RANK %" SCNd16 "] Node %" SCNd16 " wait for %" SCNd16 " nodes to establish colouring\n",
			        world_rank, v_id, wait_counters[v_id]);
		}
		delete neighIt;
	}

	int coloured_count = 0;
	while(coloured_count < v_count) {
		/* process vertices with count == 0 */
		for(int rel_v_id = 0; rel_v_id < v_count; rel_v_id++) {
			if(wait_counters[rel_v_id] == 0) {
				/* lets find smallest unused colour */
				int iter_count = 0;
				int previous_used_colour = -1;
				/* find first gap */
				for(auto used_colour: used_colours[rel_v_id]) {
					if (iter_count < used_colour) break; /* we found gap */
					previous_used_colour = used_colour;
					iter_count += 1;
				}
				int chosen_colour = previous_used_colour + 1;

				/* inform neighbours */
				int neigh_id = 0;
				Iterator<int> *neighIt = g->getNeighbourIterator(world_rank);
				while(neighIt->hasNext()) {
					neigh_id = neighIt->next();
					if (neigh_id < world_rank) {
						/* if it's larger it already has colour and is not interested */
						BufferAndRequest *b = sendBuffers.allocateBuffer();
						b->buffer.receiving_node_id = neigh_id;
						b->buffer.used_colour = chosen_colour;

						int target_node = get_node_from_vertex_id(g->getVertexCount(), world_size, neigh_id);
						MPI_Isend(&b->buffer, 1, mpi_message_type, target_node, MPI_TAG, MPI_COMM_WORLD, &b->request);
					}
					delete neighIt;
				}

				coloured_count += 1;

				/* wait for send requests and clean them up */
				sendBuffers.tryFreeBuffers([](BufferAndRequest* b) {
					int result = 0;
					MPI_Test(&b->request, &result, MPI_STATUS_IGNORE);
					if(result != 0) {
						MPI_Wait(&b->request, MPI_STATUS_IGNORE);
						return true;
					} else {
						return false;
					}
				});

			}
		}

		/* check if any outstanding receive request completed */
		int receive_result;
		for(int i = 0; i < OUTSTANDING_RECEIVE_REQUESTS; i++) {
			receive_result = 0;
			MPI_Test(receive_requests[i], &receive_result, MPI_STATUS_IGNORE);

			if(receive_result != 0) {
				/* completed */
				MPI_Wait(receive_requests[i], MPI_STATUS_IGNORE);
				int vid = receive_buffers[i].receiving_node_id;
				wait_counters[vid] -= 1;
				used_colours[vid].insert(receive_buffers[i].used_colour);

				/* post new request */
				MPI_Irecv(&receive_buffers[i], 1, mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, receive_requests[i]);
			}
		}
	}

	/* clean up */
	for(int i = 0; i < OUTSTANDING_RECEIVE_REQUESTS; i++) {
		MPI_Cancel(receive_requests[i]);
		delete receive_requests[i];
	}
	delete[] receive_requests;
	delete[] receive_buffers;

	delete[] used_colours;
	delete[] wait_counters;

	return true;
}
