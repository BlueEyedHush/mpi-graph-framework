//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPHCOLOURING_H
#define FRAMEWORK_GRAPHCOLOURING_H

#include <algorithm>
#include <cstring>
#include <cstddef>
#include <functional>
#include <set>
#include <list>
#include <unordered_map>
#include <sstream>
#include <mpi.h>
#include <glog/logging.h>
#include <algorithms/Colouring.h>
#include <utils/BufferPool.h>

namespace details { namespace GraphColouringMp {
	/* values in mB */
	const std::string IN_REQ_SOFT_OPT = "gcm-in-rs";
	const std::string IN_REQ_HARD_OPT = "gcm-in-rh";
	const std::string OUT_REQ_OPT = "gcm-out";

	struct Config {
		/* counts, not mB */
		size_t inRequestsSoft = 1000;
		size_t inRequestsHard = 2000;
		size_t outRequests = 1000;

		std::string to_string() {
			std::stringstream ss;
			ss << "GraphColouringMp | inSoft: " << inRequestsSoft << ", inHard: " << inRequestsHard
			   << ", out: " << outRequests;
			return ss.str();
		}
	};

	template <typename T>
	size_t mbToCount(size_t mbs) {
		// takes into account around 24B overhead per std::list element
		return (mbs*1024*1024)/((sizeof(T)+24)*CHAR_BIT);
	}

	template <typename TLocalId>
	Config buildConfig(ConfigMap cm, size_t v_count) {
		Config c;

		if (cm.find(IN_REQ_SOFT_OPT) != cm.end())
			c.inRequestsSoft = mbToCount<TLocalId>(std::stoull(cm[IN_REQ_SOFT_OPT]));
		if (cm.find(IN_REQ_HARD_OPT) != cm.end())
			c.inRequestsHard = mbToCount<TLocalId>(std::stoull(cm[IN_REQ_HARD_OPT]));
		if (cm.find(OUT_REQ_OPT) != cm.end()) {
			auto v = cm[OUT_REQ_OPT];
			size_t ombs = 0;

			if (v != "a")
				ombs = std::stoull(v);
			else
				ombs = std::max((size_t) 1, (v_count/9500)*4);

			c.outRequests = mbToCount<TLocalId>(ombs);
		}

		return c;
	}

	template<typename TLocalId>
	struct BufferAndRequest {
		MPI_Request request;
		Message<TLocalId> buffer;
	};
}}

/*
 * todo:
 * * swap with last (maybe don't use separate list for free buffers?
 * * keep limit of free buffers! (decide what to do in iterate?
 */

template <class TGraphPartition>
class GraphColouringMp : public GraphColouring<TGraphPartition> {
private:
	IMPORT_ALIASES(TGraphPartition)

	static const int V_LOG_LVL;

public:
	bool run(TGraphPartition *g, AAuxiliaryParams aParams) {
		using namespace details;
		using namespace details::GraphColouringMp;

		#ifdef GCM_NO_LOCAL_SHORTCIRCUIT
		LOG(INFO) << "GCM_NO_LOCAL_SHORTCIRCUIT enabled";
		#endif

		Config parsedConf = buildConfig<LocalId>(aParams.config, g->masterVerticesCount());
		LOG(INFO) << parsedConf.to_string();

		int nodeId;
		MPI_Comm_rank(MPI_COMM_WORLD, &nodeId);

		std::unordered_map<LocalId, VertexTempData*> vertexDataMap;
		this->finalColouring = new VertexColour[g->masterVerticesMaxCount()];

		MPI_Datatype mpi_message_type = Message<LocalId>::mpiDatatype();
		MPI_Type_commit(&mpi_message_type);

		BufferPool<BufferAndRequest<LocalId>> receiveBuffers(parsedConf.outRequests);

		size_t receivesFinished = 0;
		auto tryFreeReceiveCb = [&vertexDataMap, &mpi_message_type, &receivesFinished](BufferAndRequest<LocalId> *b) {
			int receive_result = 0;
			MPI_Test(&b->request, &receive_result, MPI_STATUS_IGNORE);

			if (receive_result != 0) {
				/* completed */
				MPI_Wait(&b->request, MPI_STATUS_IGNORE);
				int t_id = b->buffer.receiving_node_id;
				vertexDataMap[t_id]->wait_counter -= 1;
				vertexDataMap[t_id]->used_colours.insert(b->buffer.used_colour);
				VLOG(V_LOG_LVL) << "Received: node = " << b->buffer.receiving_node_id << ", colour = "
				                << b->buffer.used_colour;

				/* post new request */
				MPI_Irecv(&b->buffer, 1, mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &b->request);
				receivesFinished += 1;
			}

			/* one way or another, buffer doesn't change it's status */
			return false;
		};



		auto mpiWaitIfFinished = [](BufferAndRequest<LocalId> *b) {
			int result = 0;
			MPI_Test(&b->request, &result, MPI_STATUS_IGNORE);
			if (result != 0) {
				MPI_Wait(&b->request, MPI_STATUS_IGNORE);
				return true;
			} else {
				return false;
			}
		};

		auto mpiHardWaitCb = [](BufferAndRequest<LocalId> *b) {
			MPI_Wait(&b->request, MPI_STATUS_IGNORE);
		};

		#ifdef GCM_NO_LOCAL_SHORTCIRCUIT
		auto oneOffHardCb = [&receivesFinished, &tryFreeReceiveCb, &receiveBuffers]() {
			receivesFinished = 0;
			receiveBuffers.foreachUsed(tryFreeReceiveCb);
			VLOG(V_LOG_LVL-2) << "Emergency hard wait, flushed " << receivesFinished << " receive operations";
		};
		#else
		auto oneOffHardCb = []() {
			VLOG(V_LOG_LVL-2) << "Emergency hard wait, no flushing";
		};
		#endif

		AutoFreeingBuffer<BufferAndRequest<LocalId>> sendBuffers(parsedConf.inRequestsSoft, parsedConf.inRequestsHard,
		                                                         mpiWaitIfFinished, mpiHardWaitCb, oneOffHardCb);

		LOG(INFO) << "Finished initialization";

		/* start outstanding receive requests */
		receiveBuffers.foreachFree([&](BufferAndRequest<LocalId> *b) {
			MPI_Irecv(&b->buffer, 1, mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &b->request);
			return true;
		});

		LOG(INFO) << "Posted initial outstanding receive requests";

		/* gather information about rank of neighbours & initialize temporary structures */
		g->foreachMasterVertex([&](const LocalId v_id) {
			auto v_id_num = g->toNumeric(v_id);

			vertexDataMap[v_id] = new VertexTempData();

			VLOG(V_LOG_LVL) << "Looking @ " << g->idToString(v_id) << "(" << v_id_num << ") neighbours";
			g->foreachNeighbouringVertex(v_id, [&](const GlobalId neigh_id) {
				auto neigh_num = g->toNumeric(neigh_id);
				VLOG(V_LOG_LVL) << "Looking @ " << g->idToString(neigh_id) << "(" << neigh_num << ")";
				if (neigh_num > v_id_num) {
					vertexDataMap[v_id]->wait_counter++;
					VLOG(V_LOG_LVL+1) << "Qualified!";
				} else {
					VLOG(V_LOG_LVL+1) << "Rejected!";
				}

				return ITER_PROGRESS::CONTINUE;
			});

			VLOG(V_LOG_LVL) << "Waiting for " << vertexDataMap[v_id]->wait_counter << " vertices to establish colouring";

			return ITER_PROGRESS::CONTINUE;
		});

		LOG(INFO) << "Finished gathering information about neighbours";

		int coloured_count = 0;
		size_t all_count = g->masterVerticesCount();
		while(coloured_count < all_count) {
			/* process vertices with count == 0 */
			size_t coloured_this_iter = 0;
			size_t still_waiting = 0;
			g->foreachMasterVertex([&, nodeId, this](const LocalId v_id) {
				auto wc = vertexDataMap[v_id]->wait_counter;
				VLOG(V_LOG_LVL+1) << g->idToString(v_id) << " current wait_counter: " << wc;

				assert(wc >= -1);

				if(wc == 0) {
					auto v_id_num = g->toNumeric(v_id);

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
					this->finalColouring[v_id] = chosen_colour;

					VLOG(V_LOG_LVL) << "!!! All neighbours of " << g->idToString(v_id) << "(" << v_id_num
					                << ") chosen colours, we choose " << chosen_colour;

					/* inform neighbours */
					g->foreachNeighbouringVertex(v_id, [&, g, nodeId](const GlobalId neigh_id) {
						NumericId neigh_num = g->toNumeric(neigh_id);
						if (neigh_num < v_id_num) {
							/* if it's larger it already has colour and is not interested */
							auto neighNodeId = g->toMasterNodeId(neigh_id);
							auto neighLocalId = g->toLocalId(neigh_id);
							#ifndef GCM_NO_LOCAL_SHORTCIRCUIT
							if(neighNodeId == nodeId) {
								vertexDataMap[neighLocalId]->wait_counter -= 1;
								vertexDataMap[neighLocalId]->used_colours.insert(chosen_colour);
								VLOG(V_LOG_LVL+1) << g->idToString(neigh_id) << "(" << neigh_num
								          << ") is local, informing about colour "<< chosen_colour;
							} else {
							#endif
								BufferAndRequest<LocalId> *b = sendBuffers.get();
								b->buffer.receiving_node_id = neighLocalId;
								b->buffer.used_colour = chosen_colour;

								MPI_Isend(&b->buffer, 1, mpi_message_type, neighNodeId, MPI_TAG, MPI_COMM_WORLD, &b->request);

								VLOG(V_LOG_LVL+1) << "Isend to " << g->idToString(neigh_id) << "(" << neigh_num << ") info that "
								          << g->idToString(v_id) << "(" << v_id_num << ") has been coloured with "
								          << chosen_colour;
							#ifndef GCM_NO_LOCAL_SHORTCIRCUIT
							}
							#endif
						}

						return ITER_PROGRESS::CONTINUE;
					});
					VLOG(V_LOG_LVL+1) << "Informed neighbours about colour being chosen";

					vertexDataMap[v_id]->wait_counter = -1; // so that we don't process it over and over again
					coloured_this_iter += 1;
					coloured_count += 1;

					return ITER_PROGRESS::CONTINUE;
				} else if (wc != -1) {
					still_waiting += 1;
				}

				sendBuffers.tryFree();

				return ITER_PROGRESS::CONTINUE;
			});

			VLOG(V_LOG_LVL-1) << "0-wait processing finished. Coloured " << coloured_this_iter << ". On this node "
			                  << coloured_count << "/" << all_count << ". Still waiting for: " << still_waiting;

			/* check if any outstanding receive request completed */
			receivesFinished = parsedConf.outRequests; // just to enter 0 iteration of a looop
			uint32_t cycleCount = 0;
			while(receivesFinished == parsedConf.outRequests) {
				receivesFinished = 0;
				receiveBuffers.foreachUsed(tryFreeReceiveCb);
				cycleCount += 1;
			}
			VLOG(V_LOG_LVL-2) << receivesFinished << '/' << parsedConf.outRequests << " (" << cycleCount
			                  << " cycles) receives succesfully waited on";

			/* wait for send requests and clean them up */
			sendBuffers.wait();
			VLOG(V_LOG_LVL) << "Finished (for current iteration) waiting for send buffers";
		}

		/* clean up */

		/* for some reason MPI_Cancel hangs sometimes, usually when compiling with GCM_NO_LOCAL_SHORTCIRCUIT enabled
		 * since no proper cleanup is carried out, running multiple repetitions wihtin single process is probably
		 * not a good idea
		 */
		#ifndef GCM_NO_LOCAL_SHORTCIRCUIT
		receiveBuffers.foreachUsed([&](BufferAndRequest<LocalId> *b) {
			MPI_Cancel(&b->request);
			return true;
		});
		#endif

		MPI_Type_free(&mpi_message_type);

		for(auto kv: vertexDataMap) {
			delete kv.second;
		}
		LOG(INFO) << "Cleanup finished, terminating";

		return true;
	}
};

template <typename T>
const int GraphColouringMp<T>::V_LOG_LVL = 5;

#endif //FRAMEWORK_GRAPHCOLOURING_H
