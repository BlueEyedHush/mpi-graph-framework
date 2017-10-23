//
// Created by blueeyedhush on 25.05.17.
//

#ifndef FRAMEWORK_GRAPHCOLOURINGASYNC_H
#define FRAMEWORK_GRAPHCOLOURINGASYNC_H

#include <algorithm>
#include <cstring>
#include <cstddef>
#include <set>
#include <list>
#include <unordered_map>
#include <mpi.h>
#include <boost/pool/pool.hpp>
#include <boost/pool/object_pool.hpp>
#include <glog/logging.h
#include <algorithms/Colouring.h>
#include <utils/MPIAsync.h>
#include <utils/CliColours.h>

namespace details {
	/* each of those callbacks must be templetized */

	static MPI_Request* scheduleReceive(Message *b, MPI_Datatype *mpi_message_type, boost::pool<> *rqPool) {
		MPI_Request *rq = reinterpret_cast<MPI_Request *>(rqPool->malloc());
		MPI_Irecv(b, 1, *mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, rq);
		return rq;
	}

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

			LOG(INFO) << CLI_BOLD "All neighbours of " << gd->g->idToString(v_id) << "[" << v_id_num 
			          << "] chosen colours, we choose " << chosen_colour << CLI_RESET;

			/* inform neighbours */
			gd->g->forEachNeighbour(v_id, [&](GlobalVertexId neigh_id) {
				unsigned long long neigh_num = gd->g->toNumerical(neigh_id);
				if (neigh_num < v_id_num) {
					/* if it's larger it already has colour and is not interested */
					if(gd->g->isLocalVertex(neigh_id)) {
						gd->vertexDataMap->at(neigh_id.localId)->wait_counter -= 1;
						gd->vertexDataMap->at(neigh_id.localId)->used_colours.insert(chosen_colour);
						LOG(INFO) << gd->g->idToString(neigh_id) << "[" << neigh_num 
						          << "] is local, scheduling callback " << chosen_colour;

						/* it might be already processed, then it's wait_counter'll < 0 */
						if (gd->vertexDataMap->at(neigh_id.localId)->wait_counter == 0) {
							ColourVertex *cb = gd->colourVertexCbPool->construct(neigh_id.localId, gd);
							gd->am->submitTask(cb);
							LOG(INFO) << "Scheduled";
						}
					} else {
						Message *b = gd->sendPool->construct();
						b->receiving_node_id = neigh_id.localId;
						b->used_colour = chosen_colour;

						MPI_Request *rq = reinterpret_cast<MPI_Request *>(gd->mpiRequestPool->malloc());
						MPI_Isend(b, 1, *gd->mpi_message_type, neigh_id.nodeId, MPI_TAG, MPI_COMM_WORLD, rq);

						LOG(INFO) << "Isend to " << gd->g->idToString(neigh_id) << "[" << neigh_num << "] info that " 
						          << gd->g->idToString(v_id) << "[" << v_id_num << "] has been coloured with " 
						          << chosen_colour << " scheduled";

						OnSendFinished *cb = gd->sendFinishedCbPool->construct(b, gd);
						gd->am->submitWaitingTask(rq, cb);
					}
				}
			});
			LOG(INFO) << "Informed neighbours about colour being chosen";
			*gd->coloured_count += 1;

			gd->colourVertexCbPool->destroy(this);
		}
	};

	struct OnReceiveFinished : public MPIAsync::Callback {
		Message *b;
		GlobalData *gd;

		OnReceiveFinished(Message *buffer, GlobalData *globalData) : b(buffer), gd(globalData) {}

		virtual void operator()() override {
			auto t_id = b->receiving_node_id;
			LOG(INFO) << "Received: node = " << t_id << ", colour = " << b->used_colour;

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

template <class TGraphPartition>
class GraphColouringMPAsync : public GraphColouring<TestGP /*@todo*/> {
private:
	IMPORT_ALIASES(TestGP /*@todo*/)

public:
	bool run(TestGP /*@todo*/ *g) {
		using namespace details;

		int nodeId;
		MPI_Comm_rank(MPI_COMM_WORLD, &nodeId);

		std::unordered_map<int, VertexTempData*> vertexDataMap;
		finalColouring = new int[g->masterVerticesMaxCount()];

		MPI_Datatype mpi_message_type = Message::mpiDatatype();
		MPI_Type_commit(&mpi_message_type);

		boost::object_pool<Message> sendPool;
		boost::object_pool<Message> receivePool;
		boost::object_pool<OnReceiveFinished> receiveFinishedCbPool;
		boost::object_pool<OnSendFinished> sendFinishedCbPool;
		boost::object_pool<ColourVertex> colourVertexCbPool;
		boost::pool<> requestPool(sizeof(MPI_Request));

		RequestCleaner *rc = new RequestCleaner(requestPool);
		MPIAsync am(rc);
		int coloured_count = 0;

		LOG(INFO) << "Finished initialization";

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

		LOG(INFO) << "Posted initial outstanding receive requests";

		/* gather information about rank of neighbours & initialize temporary structures */
		g->foreachMasterVertex([&](const LocalId v_id) {
			auto v_id_num = g->toNumeric(v_id);

			vertexDataMap[v_id] = new VertexTempData();
			int wait_counter = 0;

			LOG(INFO) << "Looking @ " << g->idToString(v_id) << "[" << v_id_num << "] neighbours";
			g->foreachNeighbouringVertex(v_id, [&](const GlobalId neigh_id) {
				auto neigh_num = g->toNumeric(neigh_id);
				LOG(INFO) << "N: " << g->idToString(neigh_id) << "[" << neigh_num << "]";
				if (neigh_num > v_id_num) {
					wait_counter++;
					LOG(INFO) << "Qualified!";
				} else {
					LOG(INFO) << "Rejected!";
				}

				return true;
			});

			vertexDataMap[v_id]->wait_counter += wait_counter;
			if (wait_counter == 0) {
				am.submitTask(colourVertexCbPool.construct(v_id, &globalData));
			}

			LOG(INFO) << "Waiting for " << vertexDataMap[v_id]->wait_counter << " nodes to establish colouring";
		});

		LOG(INFO) << "Finished gathering information about neighbours";

		while(coloured_count < g->masterVerticesCount()) {
			/* check if any outstanding receive request completed */
			am.pollAll();
			LOG(INFO) << "Finished current iteration of task queue processing";
		}

		/* clean up */
		am.shutdown();

		MPI_Type_free(&mpi_message_type);

		for(auto kv: vertexDataMap) {
			delete kv.second;
		}
		LOG(INFO) << "Cleanup finished, terminating";

		return true;
	}
};


#endif //FRAMEWORK_GRAPHCOLOURINGASYNC_H
