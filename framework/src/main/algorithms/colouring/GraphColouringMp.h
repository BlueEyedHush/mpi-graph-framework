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

namespace details {
	template<typename TLocalId>
	struct BufferAndRequest {
		MPI_Request request;
		Message<TLocalId> buffer;
	};
}

template <class TGraphPartition>
class GraphColouringMp : public GraphColouring<TGraphPartition> {
private:
	IMPORT_ALIASES(TGraphPartition)

public:
	bool run(TGraphPartition *g) {
		using namespace details;

		int nodeId;
		MPI_Comm_rank(MPI_COMM_WORLD, &nodeId);

		std::unordered_map<LocalId, VertexTempData*> vertexDataMap;
		this->finalColouring = new VertexColour[g->masterVerticesMaxCount()];

		MPI_Datatype mpi_message_type = Message<LocalId>::mpiDatatype();
		MPI_Type_commit(&mpi_message_type);

		BufferPool<BufferAndRequest<LocalId>> sendBuffers;
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

			LOG(INFO) << "Looking @ " << g->idToString(v_id) << "[" << v_id_num << "] neighbours";
			g->foreachNeighbouringVertex(v_id, [&](const GlobalId neigh_id) {
				auto neigh_num = g->toNumeric(neigh_id);
				LOG(INFO) << "Looking @ " << g->idToString(neigh_id) << "[" << neigh_num << "]";
				if (neigh_num > v_id_num) {
					vertexDataMap[v_id]->wait_counter++;
					LOG(INFO) << "Qualified!";
				} else {
					LOG(INFO) << "Rejected!";
				}

				return true;
			});

			LOG(INFO) << "Waiting for %d nodes to establish colouring";

			return true;
		});

		LOG(INFO) << "Finished gathering information about neighbours";

		int coloured_count = 0;
		while(coloured_count < g->masterVerticesCount()) {
			/* process vertices with count == 0 */
			g->foreachMasterVertex([&, nodeId, this](const LocalId v_id) {
				if(vertexDataMap[v_id]->wait_counter == 0) {
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

					LOG(INFO) << "!!! All neighbours of " << g->idToString(v_id) << "[" << v_id_num
					          << "] chosen colours, we choose " << chosen_colour;

					/* inform neighbours */
					g->foreachNeighbouringVertex(v_id, [&, g, nodeId](const GlobalId neigh_id) {
						unsigned long long neigh_num = g->toNumeric(neigh_id);
						if (neigh_num < v_id_num) {
							/* if it's larger it already has colour and is not interested */
							if(g->toMasterNodeId(neigh_id) == nodeId) {
								vertexDataMap[neigh_id.localId]->wait_counter -= 1;
								vertexDataMap[neigh_id.localId]->used_colours.insert(chosen_colour);
								LOG(INFO) << g->idToString(neigh_id) << "[" << neigh_num
								          << "] is local, informing about colour "<< chosen_colour;
							} else {
								BufferAndRequest<LocalId> *b = sendBuffers.getNew();
								b->buffer.receiving_node_id = neigh_id.localId;
								b->buffer.used_colour = chosen_colour;

								MPI_Isend(&b->buffer, 1, mpi_message_type, neigh_id.nodeId, MPI_TAG, MPI_COMM_WORLD, &b->request);

								LOG(INFO) << "Isend to " << g->idToString(neigh_id) << "[" << neigh_num << "] info that "
								          << g->idToString(v_id) << "[" << v_id_num << "] has been coloured with "
								          << chosen_colour;
							}
						}

						return true;
					});
					LOG(INFO) << "Informed neighbours about colour being chosen";

					vertexDataMap[v_id]->wait_counter = -1;
					coloured_count += 1;

					return true;
				}
			});
			LOG(INFO) << "Finished processing of 0-wait-count vertices";

			/* check if any outstanding receive request completed */
			int receive_result;
			receiveBuffers.foreachUsed([&](BufferAndRequest<LocalId> *b) {
				receive_result = 0;
				MPI_Test(&b->request, &receive_result, MPI_STATUS_IGNORE);

				if (receive_result != 0) {
					/* completed */
					MPI_Wait(&b->request, MPI_STATUS_IGNORE);
					int t_id = b->buffer.receiving_node_id;
					vertexDataMap[t_id]->wait_counter -= 1;
					vertexDataMap[t_id]->used_colours.insert(b->buffer.used_colour);
					LOG(INFO) << "Received: node = " << b->buffer.receiving_node_id << ", colour = " << b->buffer.used_colour;

					/* post new request */
					MPI_Irecv(&b->buffer, 1, mpi_message_type, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &b->request);
				}

				/* one way or another, buffer doesn't change it's status */
				return false;
			});
			LOG(INFO) << "Finished (for current iteration) processing of outstanding receive requests";

			/* wait for send requests and clean them up */
			sendBuffers.foreachUsed([](BufferAndRequest<LocalId> *b) {
				int result = 0;
				MPI_Test(&b->request, &result, MPI_STATUS_IGNORE);
				if (result != 0) {
					MPI_Wait(&b->request, MPI_STATUS_IGNORE);
					return true;
				} else {
					return false;
				}
			});
			LOG(INFO) << "Finished (for current iteration) waiting for send buffers";
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

#endif //FRAMEWORK_GRAPHCOLOURING_H
