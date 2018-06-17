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
#include <mpi.h>
#include <glog/logging.h>
#include <algorithms/Colouring.h>
#include <utils/BufferPool.h>

#define GCM_LOCAL_SHORTCIRCUIT 0

namespace details {
	template<typename TLocalId>
	struct BufferAndRequest {
		MPI_Request request;
		Message<TLocalId> buffer;
	};
}

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

		int nodeId;
		MPI_Comm_rank(MPI_COMM_WORLD, &nodeId);

		std::unordered_map<LocalId, VertexTempData*> vertexDataMap;
		this->finalColouring = new VertexColour[g->masterVerticesMaxCount()];

		MPI_Datatype mpi_message_type = Message<LocalId>::mpiDatatype();
		MPI_Type_commit(&mpi_message_type);

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

		size_t sendSoft = 1000;
		AutoFreeingBuffer<BufferAndRequest<LocalId>> sendBuffers(sendSoft, 2*sendSoft, mpiWaitIfFinished, mpiHardWaitCb);
		BufferPool<BufferAndRequest<LocalId>> receiveBuffers(OUTSTANDING_RECEIVE_REQUESTS);

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
							#if GCM_LOCAL_SHORTCIRCUIT == 1
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
							#if GCM_LOCAL_SHORTCIRCUIT == 1
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

			VLOG(V_LOG_LVL-2) << "0-wait processing finished. Coloured " << coloured_this_iter << ". On this node "
			                  << coloured_count << "/" << all_count << ". Still waiting for: " << still_waiting;

			/* check if any outstanding receive request completed */
			size_t receivesFinished = 0;
			receiveBuffers.foreachUsed([&vertexDataMap, &mpi_message_type, &receivesFinished](BufferAndRequest<LocalId> *b) {
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
			});
			VLOG(V_LOG_LVL-1) << receivesFinished << '/' << OUTSTANDING_RECEIVE_REQUESTS
			                  << " receives succesfully waited on during current iteration";

			/* wait for send requests and clean them up */
			sendBuffers.tryFree();
			VLOG(V_LOG_LVL) << "Finished (for current iteration) waiting for send buffers";
		}

		/* clean up */
		receiveBuffers.foreachUsed([&](BufferAndRequest<LocalId> *b) {
			MPI_Cancel(&b->request);
			return true;
		});

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
