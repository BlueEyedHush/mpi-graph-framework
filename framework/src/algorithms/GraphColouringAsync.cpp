//
// Created by blueeyedhush on 25.05.17.
//

#include "GraphColouringAsync.h"

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <cstddef>
#include <set>
#include <list>
#include <unordered_map>
#include <mpi.h>
#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>
#include <utils/MPIAsync.h>
#include <utils/CliColours.h>
#include "shared.h"

#define MPI_TAG 0
#define OUTSTANDING_RECEIVE_REQUESTS 5

static MPI_Request* scheduleReceive(Message *b, MPI_Datatype *mpi_message_type, boost::pool<> *rqPool) {
	MPI_Request *rq = reinterpret_cast<MPI_Request *>(rqPool->malloc());
	MPI_Irecv(b, 1, *mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, rq);
	return rq;
}


namespace {
	class OnReceiveFinished;
	class OnSendFinished;
	class ColourVertex;

	struct GlobalData {
		std::unordered_map<int, VertexTempData*> *vertexDataMap;
		int nodeId;
		MPI_Datatype *mpi_message_type;
		GraphPartition *g;
		int *coloured_count;
		MPIAsync *am;

		boost::object_pool<Message> *sendPool;
		boost::object_pool<OnReceiveFinished> *receiveFinishedCbPool;
		boost::object_pool<OnSendFinished> *sendFinishedCbPool;
		boost::object_pool<ColourVertex> *colourVertexCbPool;
		boost::pool<> *mpiRequestPool;

		int *finalColouring;
	};

	struct OnSendFinished : public MPIAsync::Callback {
		Message *b;
		GlobalData *gd;

		OnSendFinished(Message *b, GlobalData *gd) : b(b), gd(gd) {}

		virtual void operator()() override {
			gd->sendPool->destroy(b);
			gd->sendFinishedCbPool->destroy(this);
		}
	};

	struct ColourVertex : public MPIAsync::Callback {
		int v_id;
		GlobalData *gd;

		ColourVertex(int v_id, GlobalData *gd) : v_id(v_id), gd(gd) {}

		virtual void operator()() override {
			GlobalVertexId globalForVId(gd->nodeId, v_id);
			unsigned long long v_id_num = gd->g->toNumerical(globalForVId);

			/* lets find smallest unused colour */
			int iter_count = 0;
			int previous_used_colour = -1;
			/* find first gap */
			for(auto used_colour: gd->vertexDataMap->at(v_id)->used_colours) {
				if (iter_count < used_colour) break; /* we found gap */
				previous_used_colour = used_colour;
				iter_count += 1;
			}
			int chosen_colour = previous_used_colour + 1;
			gd->finalColouring[v_id] = chosen_colour;

			gd->vertexDataMap->at(v_id)->wait_counter = -1;

			fprintf(stderr, CLI_BOLD "[%d] All neighbours of (%d, %d, %llu) chosen colours, we choose %d\n" CLI_RESET,
			        gd->nodeId, gd->nodeId, v_id, v_id_num, chosen_colour);

			/* inform neighbours */
			gd->g->forEachNeighbour(v_id, [&](GlobalVertexId neigh_id) {
				unsigned long long neigh_num = gd->g->toNumerical(neigh_id);
				if (neigh_num < v_id_num) {
					/* if it's larger it already has colour and is not interested */
					if(gd->g->isLocalVertex(neigh_id)) {
						gd->vertexDataMap->at(neigh_id.localId)->wait_counter -= 1;
						gd->vertexDataMap->at(neigh_id.localId)->used_colours.insert(chosen_colour);
						fprintf(stderr, "[%d] (%d,%d,%llu) is local, scheduling callback %d\n", gd->nodeId,
						        neigh_id.nodeId, neigh_id.localId, neigh_num, chosen_colour);

						/* it might be already processed, then it's wait_counter'll < 0 */
						if (gd->vertexDataMap->at(neigh_id.localId)->wait_counter == 0) {
							ColourVertex *cb = gd->colourVertexCbPool->construct(neigh_id.localId, gd);
							gd->am->submitTask(cb);
							fprintf(stderr, "[%d] Scheduled\n", gd->nodeId);
						}
					} else {
						Message *b = gd->sendPool->construct();
						b->receiving_node_id = neigh_id.localId;
						b->used_colour = chosen_colour;

						MPI_Request *rq = reinterpret_cast<MPI_Request *>(gd->mpiRequestPool->malloc());
						MPI_Isend(b, 1, *gd->mpi_message_type, neigh_id.nodeId, MPI_TAG, MPI_COMM_WORLD, rq);

						fprintf(stderr, "[%d] Isend to (%d,%d,%llu) info that (%d,%d,%llu) has been coloured with %d scheduled\n",
						        gd->nodeId, neigh_id.nodeId, neigh_id.localId, neigh_num, gd->nodeId, v_id, v_id_num, chosen_colour);

						OnSendFinished *cb = gd->sendFinishedCbPool->construct(b, gd);
						gd->am->submitWaitingTask(rq, cb);
					}
				}
			});
			fprintf(stderr, "[%d] Informed neighbours about colour being chosen\n", gd->nodeId);
			*gd->coloured_count += 1;

			gd->colourVertexCbPool->destroy(this);
		}
	};

	struct OnReceiveFinished : public MPIAsync::Callback {
		Message *b;
		GlobalData *gd;

		OnReceiveFinished(Message *buffer, GlobalData *globalData) : b(buffer), gd(globalData) {}

		virtual void operator()() override {
			int t_id = b->receiving_node_id;
			fprintf(stderr, "[%d] Received: node = %d, colour = %d\n", gd->nodeId, b->receiving_node_id, b->used_colour);

			gd->vertexDataMap->at(t_id)->wait_counter -= 1;
			gd->vertexDataMap->at(t_id)->used_colours.insert(b->used_colour);

			/* post new request */
			MPI_Request *rq = scheduleReceive(b, gd->mpi_message_type, gd->mpiRequestPool);
			OnReceiveFinished *cb = gd->receiveFinishedCbPool->construct(b, gd);
			gd->am->submitWaitingTask(rq, cb);

			if(gd->vertexDataMap->at(t_id)->wait_counter == 0) {
			/* handle if count decreased to 0, and is not below 0 - this happens in case of already processed vertices */
				ColourVertex *cb = gd->colourVertexCbPool->construct(t_id, gd);
				gd->am->submitTask(cb);
			}

			gd->receiveFinishedCbPool->destroy(this);
		}
	};

	struct RequestCleaner : public MPIAsync::MPIRequestCleaner {
		boost::pool<>& rqPool;

		RequestCleaner(boost::pool<>& requestPool) : rqPool(requestPool) {}

		virtual void operator()(MPI_Request* r) {
			rqPool.free(r);
		};
	};
}



bool GraphColouringMPAsync::run(GraphPartition *g) {
	int nodeId = g->getNodeId();

	std::unordered_map<int, VertexTempData*> vertexDataMap;
	finalColouring = new int[g->getLocalVertexCount()];

	MPI_Datatype mpi_message_type;
	register_mpi_message(&mpi_message_type);

	boost::object_pool<Message> sendPool;
	boost::object_pool<Message> receivePool;
	boost::object_pool<OnReceiveFinished> receiveFinishedCbPool;
	boost::object_pool<OnSendFinished> sendFinishedCbPool;
	boost::object_pool<ColourVertex> colourVertexCbPool;
	boost::pool<> requestPool(sizeof(MPI_Request));

	RequestCleaner *rc = new RequestCleaner(requestPool);
	MPIAsync am(rc);
	int coloured_count = 0;

	fprintf(stderr, "[%d] Finished initialization\n", nodeId);

	GlobalData globalData;
	globalData.am = &am;
	globalData.coloured_count = &coloured_count;
	globalData.g = g;
	globalData.mpi_message_type = &mpi_message_type;
	globalData.nodeId = nodeId;
	globalData.vertexDataMap = &vertexDataMap;
	globalData.sendPool = &sendPool;
	globalData.receiveFinishedCbPool = &receiveFinishedCbPool;
	globalData.sendFinishedCbPool = &sendFinishedCbPool;
	globalData.colourVertexCbPool = &colourVertexCbPool;
	globalData.mpiRequestPool = &requestPool;
	globalData.finalColouring = finalColouring;

	/* start outstanding receive requests */
	for(int i = 0; i < OUTSTANDING_RECEIVE_REQUESTS; i++) {
		Message *b = receivePool.construct();
		MPI_Request *rq = scheduleReceive(b, &mpi_message_type, &requestPool);
		am.submitWaitingTask(rq, receiveFinishedCbPool.construct(b, &globalData));
	}

	fprintf(stderr, "[%d] Posted initial outstanding receive requests\n", nodeId);

	/* gather information about rank of neighbours & initialize temporary structures */
	g->forEachLocalVertex([&](LocalVertexId v_id) {
		GlobalVertexId globalForVId(nodeId, v_id);
		unsigned long long v_id_num = g->toNumerical(globalForVId);

		vertexDataMap[v_id] = new VertexTempData();
		int wait_counter = 0;

		fprintf(stderr, "[%d] Looking @ (%d, %d, %llu) neighbours\n", nodeId, nodeId, v_id, v_id_num);
		g->forEachNeighbour(v_id, [&](GlobalVertexId neigh_id) {
			unsigned long long neigh_num = g->toNumerical(neigh_id);
			fprintf(stderr, "[%d] N: (%d, %d, %llu)\n", nodeId, neigh_id.nodeId, neigh_id.localId, neigh_num);
			if (neigh_num > v_id_num) {
				wait_counter++;
				fprintf(stderr, "[%d] Qualified!\n", nodeId);
			} else {
				fprintf(stderr, "[%d] Rejected!\n", nodeId);
			}
		});

		vertexDataMap[v_id]->wait_counter += wait_counter;
		if (wait_counter == 0) {
			am.submitTask(colourVertexCbPool.construct(v_id, &globalData));
		}

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

	return true;
}

int *GraphColouringMPAsync::getResult() {
	return finalColouring;
}

GraphColouringMPAsync::GraphColouringMPAsync() {
	finalColouring = nullptr;
}

GraphColouringMPAsync::~GraphColouringMPAsync() {
	if (finalColouring != nullptr) delete[] finalColouring;
}

