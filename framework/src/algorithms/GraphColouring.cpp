//
// Created by blueeyedhush on 06.05.17.
//

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <cstddef>
#include <functional>
#include <set>
#include <list>
#include <unordered_map>
#include <mpi.h>
#include "GraphColouring.h"
#include <utils/BufferPool.h>

/*
 * @ToDo:
 * - dynamically adjust number of outstanding requests
 * - same functions for receive & send pools, same function for spawning new outstanding receive requests
 * - iterate over vertices - use foreach
 * - another list for 0ed vertices
 * - look at memory usage (valgrind, review types, use pointers for buffers and cleanup asap, free send buffers, free requests & receive buffers)
 * - unit tests
 * - correct error handling
 * - try to replace cleanup loops with waitsome or test_any/all
 * - is MPI_Wait needed with MPI_Test?
 * - pass std::functions by reference
 */

#define MPI_TAG 0
#define OUTSTANDING_RECEIVE_REQUESTS 5


struct Message {
	LocalVertexId receiving_node_id;
	int used_colour;
};

void register_mpi_message(MPI_Datatype *memory) {
	int blocklengths[] = {1, 1};
	MPI_Aint displacements[] = {offsetof(Message, receiving_node_id), offsetof(Message, used_colour)};
	MPI_Datatype building_types[] = {MPI_INT, MPI_INT};
	MPI_Type_create_struct(2, blocklengths, displacements, building_types, memory);

	MPI_Type_commit(memory);
}

struct VertexTempData {
	VertexTempData() : wait_counter(0) {}

	std::set<int> used_colours;
	int wait_counter;
};

struct BufferAndRequest {
	MPI_Request request;
	Message buffer;
};


class MPIAsync {
public:
	MPIAsync() {
		nextToProcess = 0;
		requests = new std::vector<MPI_Request*>();
		callbacks = new std::vector<std::function<void(void)>>();
	}

	/**
	 *
	 * @param request - takes ownership of the memory and'll clean it up
	 */
	void callWhenFinished(MPI_Request *request, const std::function<void(void)> callback) {
		requests->push_back(request);
		callbacks->push_back(callback);
	}

	void pollNext(int x) {
		for(int i = 0; i < x; i++) {
			MPI_Request *rq = requests->at(i);

			if (rq == nullptr) {
				/* no need to wait, can execute immediatelly */
				executeCallbackAt(rq, i);
			} else {
				int result = 0;
				MPI_Test(rq, &result, MPI_STATUS_IGNORE);
				if (result != 0) {
					MPI_Wait(rq, MPI_STATUS_IGNORE);

					executeCallbackAt(rq, i);
				}
			}

			if (nextToProcess >= requests->size()) {
				nextToProcess = 0;
			}
		}
	}

	void pollAll() {
		int toPoll = requests->size() - nextToProcess;
		pollNext(toPoll);
	}

	void shutdown() {
		for(int i = 0; i < requests->size(); i++) {
			MPI_Request *rq = requests->at(i);
			if (rq != nullptr) {
				MPI_Cancel(rq);
			}
		}
	}

private:
	int nextToProcess;
	std::vector<MPI_Request*> *requests;
	std::vector<std::function<void(void)>> *callbacks;

	void executeCallbackAt(MPI_Request *rq, int i) {
		auto cb = callbacks->at(i);
		cb();

		if (requests->size() >= 2) {
			size_t lastIdx = requests->size() - 1;

			requests->at(i) = requests->at(lastIdx);
			callbacks->at(i) = callbacks->at(lastIdx);

			requests->pop_back();
			callbacks->pop_back();
		} else {
			requests->clear();
			callbacks->clear();
		}

		if (rq != nullptr) {
			delete rq;
		}
	}
};


bool GraphColouringMP::run(GraphPartition *g) {
	int nodeId = g->getNodeId();

	std::unordered_map<int, VertexTempData*> vertexDataMap;

	MPI_Datatype mpi_message_type;
	register_mpi_message(&mpi_message_type);

	BufferPool<BufferAndRequest> sendBuffers;
	BufferPool<BufferAndRequest> receiveBuffers(OUTSTANDING_RECEIVE_REQUESTS);

	fprintf(stderr, "[%d] Finished initialization\n", nodeId);

	/* start outstanding receive requests */
	receiveBuffers.foreachFree([&](BufferAndRequest *b) {
		MPI_Irecv(&b->buffer, 1, mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &b->request);
		return true;
	});

	fprintf(stderr, "[%d] Posted initial outstanding receive requests\n", nodeId);

	/* gather information about rank of neighbours & initialize temporary structures */
	g->forEachLocalVertex([&](LocalVertexId v_id) {
		GlobalVertexId globalForVId(nodeId, v_id);
		unsigned long long v_id_num = g->toNumerical(globalForVId);

		vertexDataMap[v_id] = new VertexTempData();

		fprintf(stderr, "[%d] Looking @ (%d, %d, %llu) neighbours\n", nodeId, nodeId, v_id, v_id_num);
		g->forEachNeighbour(v_id, [&](GlobalVertexId neigh_id) {
			unsigned long long neigh_num = g->toNumerical(neigh_id);
			fprintf(stderr, "[%d] Looking @ (%d,%d,%llu)\n", nodeId, neigh_id.nodeId, neigh_id.localId, neigh_num);
			if (neigh_num > v_id_num) {
				vertexDataMap[v_id]->wait_counter++;
				fprintf(stderr, "[%d] Qualified!\n", nodeId);
			} else {
				fprintf(stderr, "[%d] Rejected!\n", nodeId);
			}
		});

		fprintf(stderr, "[%d] Waiting for %d nodes to establish colouring\n", nodeId, vertexDataMap[v_id]->wait_counter);
	});

	fprintf(stderr, "[%d] Finished gathering information about neighbours\n", nodeId);

	int coloured_count = 0;
	while(coloured_count < g->getLocalVertexCount()) {
		/* process vertices with count == 0 */
		g->forEachLocalVertex([&](LocalVertexId v_id) {
			if(vertexDataMap[v_id]->wait_counter == 0) {
				GlobalVertexId globalForVId(nodeId, v_id);
				unsigned long long v_id_num = g->toNumerical(globalForVId);

				/* lets find smallest unused colour */
				int iter_count = 0;
				int previous_used_colour = -1;
				/* find first gap */
				for(auto used_colour: vertexDataMap[v_id]->used_colours) {
					if (iter_count < used_colour) break; /* we found gap */
					previous_used_colour = used_colour;
					iter_count += 1;
				}
				int chosen_colour = previous_used_colour + 1;

				fprintf(stderr, "!!! [%d] All neighbours of (%d, %d,%llu) chosen colours, we choose %d\n", nodeId, nodeId,
				        v_id, v_id_num, chosen_colour);

				/* inform neighbours */
				g->forEachNeighbour(v_id, [&](GlobalVertexId neigh_id) {
					unsigned long long neigh_num = g->toNumerical(neigh_id);
					if (neigh_num < v_id_num) {
						/* if it's larger it already has colour and is not interested */
						if(g->isLocalVertex(neigh_id)) {
							vertexDataMap[neigh_id.localId]->wait_counter -= 1;
							vertexDataMap[neigh_id.localId]->used_colours.insert(chosen_colour);
							fprintf(stderr, "[%d] (%d,%d,%llu) is local, informing about colour %d\n", nodeId,
							        neigh_id.nodeId, neigh_id.localId, neigh_num, chosen_colour);
						} else {
							BufferAndRequest *b = sendBuffers.getNew();
							b->buffer.receiving_node_id = neigh_id.localId;
							b->buffer.used_colour = chosen_colour;

							MPI_Isend(&b->buffer, 1, mpi_message_type, neigh_id.nodeId, MPI_TAG, MPI_COMM_WORLD, &b->request);

							fprintf(stderr, "[%d] Isend to (%d,%d,%llu) info that (%d,%d,%llu) has been coloured with %d\n",
							        nodeId, neigh_id.nodeId, neigh_id.localId, neigh_num, nodeId, v_id, v_id_num, chosen_colour);
						}
					}
				});
				fprintf(stderr, "[%d] Informed neighbours about colour being chosen\n", nodeId);

				vertexDataMap[v_id]->wait_counter = -1;
				coloured_count += 1;
			}
		});
		fprintf(stderr, "[%d] Finished processing of 0-wait-count vertices\n", nodeId);

		/* check if any outstanding receive request completed */
		int receive_result;
		receiveBuffers.foreachUsed([&](BufferAndRequest *b) {
			receive_result = 0;
			MPI_Test(&b->request, &receive_result, MPI_STATUS_IGNORE);

			if (receive_result != 0) {
				/* completed */
				MPI_Wait(&b->request, MPI_STATUS_IGNORE);
				int t_id = b->buffer.receiving_node_id;
				vertexDataMap[t_id]->wait_counter -= 1;
				vertexDataMap[t_id]->used_colours.insert(b->buffer.used_colour);
				fprintf(stderr, "[%d] Received: node = %d, colour = %d\n", nodeId, b->buffer.receiving_node_id,
				        b->buffer.used_colour);

				/* post new request */
				MPI_Irecv(&b->buffer, 1, mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &b->request);
			}

			/* one way or another, buffer doesn't change it's status */
			return false;
		});
		fprintf(stderr, "[%d] Finished (for current iteration) processing of outstanding receive requests\n", nodeId);

		/* wait for send requests and clean them up */
		sendBuffers.foreachUsed([](BufferAndRequest *b) {
			int result = 0;
			MPI_Test(&b->request, &result, MPI_STATUS_IGNORE);
			if (result != 0) {
				MPI_Wait(&b->request, MPI_STATUS_IGNORE);
				return true;
			} else {
				return false;
			}
		});
		fprintf(stderr, "[%d] Finished (for current iteration) waiting for send buffers\n", nodeId);
	}

	/* clean up */
	receiveBuffers.foreachUsed([&](BufferAndRequest *b) {
		MPI_Cancel(&b->request);
		return true;
	});

	for(auto kv: vertexDataMap) {
		delete kv.second;
	}
	fprintf(stderr, "[%d] Cleanup finished, terminating\n", nodeId);

	return true;
}


MPI_Request* scheduleReceive(Message *b, MPI_Datatype *mpi_message_type) {
	MPI_Request *rq = new MPI_Request;
	MPI_Irecv(b, 1, *mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, rq);
	return rq;
}


std::function<void(void)> buildOnISendFinishedHandler(Message *b) {
	return [](){
		/* free buffer */
	};
}


std::function<void(void)> buildVertexColourer(int v_id,
                                              std::unordered_map<int, VertexTempData*> *vertexDataMap,
                                              int nodeId,
                                              MPI_Datatype *mpi_message_type,
                                              GraphPartition *g,
                                              BufferPool<Message> *sendPool,
                                              int *coloured_count,
                                              MPIAsync *am) {
	return [=]() {
		GlobalVertexId globalForVId(nodeId, v_id);
		unsigned long long v_id_num = g->toNumerical(globalForVId);

		/* lets find smallest unused colour */
		int iter_count = 0;
		int previous_used_colour = -1;
		/* find first gap */
		for(auto used_colour: vertexDataMap->at(v_id)->used_colours) {
			if (iter_count < used_colour) break; /* we found gap */
			previous_used_colour = used_colour;
			iter_count += 1;
		}
		int chosen_colour = previous_used_colour + 1;

		fprintf(stderr, "!!! [%d] All neighbours of (%d, %d,%llu) chosen colours, we choose %d\n", nodeId, nodeId,
		        v_id, v_id_num, chosen_colour);

		/* inform neighbours */
		g->forEachNeighbour(v_id, [&](GlobalVertexId neigh_id) {
			unsigned long long neigh_num = g->toNumerical(neigh_id);
			if (neigh_num < v_id_num) {
				/* if it's larger it already has colour and is not interested */
				if(g->isLocalVertex(neigh_id)) {
					vertexDataMap->at(neigh_id.localId)->wait_counter -= 1;
					vertexDataMap->at(neigh_id.localId)->used_colours.insert(chosen_colour);
					fprintf(stderr, "[%d] (%d,%d,%llu) is local, scheduling callback %d\n", nodeId,
					        neigh_id.nodeId, neigh_id.localId, neigh_num, chosen_colour);
					am->callWhenFinished(nullptr, buildVertexColourer(v_id, vertexDataMap, nodeId, mpi_message_type, g, sendPool, coloured_count, am));
				} else {
					Message *b = sendPool->getNew();
					b->receiving_node_id = neigh_id.localId;
					b->used_colour = chosen_colour;

					MPI_Request *rq = new MPI_Request;
					MPI_Isend(b, 1, *mpi_message_type, neigh_id.nodeId, MPI_TAG, MPI_COMM_WORLD, rq);

					fprintf(stderr, "[%d] Isend to (%d,%d,%llu) info that (%d,%d,%llu) has been coloured with %d scheduled\n",
					        nodeId, neigh_id.nodeId, neigh_id.localId, neigh_num, nodeId, v_id, v_id_num, chosen_colour);

					am->callWhenFinished(rq, buildOnISendFinishedHandler(b));
				}
			}
		});
		fprintf(stderr, "[%d] Informed neighbours about colour being chosen\n", nodeId);
		*coloured_count += 1;
	};
}


std::function<void(void)> buildOnReceiveHandler(Message *b,
                                                std::unordered_map<int, VertexTempData*> *vertexDataMap,
                                                int nodeId,
                                                MPI_Datatype *mpi_message_type,
                                                GraphPartition *g,
                                                BufferPool<Message> *sendPool,
                                                int *coloured_count,
                                                MPIAsync *am) {
	return [=]() {
		/* completed */
		int t_id = b->receiving_node_id;
		vertexDataMap->at(t_id)->wait_counter -= 1;
		vertexDataMap->at(t_id)->used_colours.insert(b->used_colour);
		fprintf(stderr, "[%d] Received: node = %d, colour = %d\n", nodeId, b->receiving_node_id, b->used_colour);

		/* post new request */
		MPI_Request *rq = scheduleReceive(b, mpi_message_type);
		am->callWhenFinished(rq, buildOnReceiveHandler(b, vertexDataMap, nodeId, mpi_message_type, g, sendPool, coloured_count, am));

		/* handle if count decreased to 0 */
		if(vertexDataMap->at(t_id)->wait_counter == 0) {
			am->callWhenFinished(nullptr, buildVertexColourer(t_id, vertexDataMap, nodeId, mpi_message_type, g, sendPool, coloured_count, am));
		}
	};
}

bool GraphColouringMPAsync::run(GraphPartition *g) {
	int nodeId = g->getNodeId();

	std::unordered_map<int, VertexTempData*> vertexDataMap;

	MPI_Datatype mpi_message_type;
	register_mpi_message(&mpi_message_type);

	MPIAsync am;
	BufferPool<Message> sendBuffers;
	BufferPool<Message> receiveBuffers(OUTSTANDING_RECEIVE_REQUESTS);

	int coloured_count = 0;

	fprintf(stderr, "[%d] Finished initialization\n", nodeId);

	/* start outstanding receive requests */
	for(int i = 0; i < OUTSTANDING_RECEIVE_REQUESTS; i++) {
		Message *b = receiveBuffers.getNew();
		MPI_Request *rq = scheduleReceive(b, &mpi_message_type);
		am.callWhenFinished(rq, buildOnReceiveHandler(b, &vertexDataMap, nodeId, &mpi_message_type, g, &sendBuffers, &coloured_count, &am));
	}

	fprintf(stderr, "[%d] Posted initial outstanding receive requests\n", nodeId);

	/* gather information about rank of neighbours & initialize temporary structures */
	g->forEachLocalVertex([&](LocalVertexId v_id) {
		GlobalVertexId globalForVId(nodeId, v_id);
		unsigned long long v_id_num = g->toNumerical(globalForVId);

		vertexDataMap[v_id] = new VertexTempData();

		fprintf(stderr, "[%d] Looking @ (%d, %d, %llu) neighbours\n", nodeId, nodeId, v_id, v_id_num);
		g->forEachNeighbour(v_id, [&](GlobalVertexId neigh_id) {
			unsigned long long neigh_num = g->toNumerical(neigh_id);
			fprintf(stderr, "[%d] Looking @ (%d,%d,%llu)\n", nodeId, neigh_id.nodeId, neigh_id.localId, neigh_num);
			if (neigh_num > v_id_num) {
				vertexDataMap[v_id]->wait_counter++;
				fprintf(stderr, "[%d] Qualified!\n", nodeId);
			} else {
				fprintf(stderr, "[%d] Rejected!\n", nodeId);
			}
		});

		fprintf(stderr, "[%d] Waiting for %d nodes to establish colouring\n", nodeId, vertexDataMap[v_id]->wait_counter);
	});

	fprintf(stderr, "[%d] Finished gathering information about neighbours\n", nodeId);

	while(coloured_count < g->getLocalVertexCount()) {
		/* check if any outstanding receive request completed */
		am.pollAll();
		fprintf(stderr, "[%d] Finished current iteration of task queue processing\n", nodeId);
	}

	/* clean up */
	am.shutdown();

	for(auto kv: vertexDataMap) {
		delete kv.second;
	}
	fprintf(stderr, "[%d] Cleanup finished, terminating\n", nodeId);

	return false;
}

bool GraphColouringRMA::run(GraphPartition *g) {

}

