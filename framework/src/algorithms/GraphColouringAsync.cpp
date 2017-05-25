//
// Created by blueeyedhush on 25.05.17.
//

#include "GraphColouringAsync.h"

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <cstddef>
#include <functional>
#include <set>
#include <list>
#include <unordered_map>
#include <mpi.h>
#include <utils/BufferPool.h>
#include <utils/MPIAsync.h>
#include "shared.h"

#define MPI_TAG 0
#define OUTSTANDING_RECEIVE_REQUESTS 5

static MPI_Request* scheduleReceive(Message *b, MPI_Datatype *mpi_message_type) {
	MPI_Request *rq = new MPI_Request;
	MPI_Irecv(b, 1, *mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, rq);
	return rq;
}


static std::function<void(void)> buildOnISendFinishedHandler(Message *b) {
	return [](){
		/* free buffer */
	};
}


static std::function<void(void)> buildVertexColourer(int v_id,
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


static std::function<void(void)> buildOnReceiveHandler(Message *b,
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
