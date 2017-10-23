//
// Created by blueeyedhush on 19.10.17.
//

#ifndef FRAMEWORK_BFSFIXEDMESSAGE_H
#define FRAMEWORK_BFSFIXEDMESSAGE_H

#include <algorithms/Bfs.h>

template <class TGraphPartition>
class Bfs_Mp_FixedMsgLen_1D_2CommRounds : public Bfs<TGraphPartition> {
private:
	IMPORT_ALIASES(TGraphPartition)
	using VertexM = details::fixedLen::VertexMessage<LocalId, GlobalId>;

public:
	const static int SEND_TAG = 1;

	Bfs_Mp_FixedMsgLen_1D_2CommRounds(const GlobalId _bfsRoot) : Bfs<TGraphPartition>(_bfsRoot) {};

	bool run(TGraphPartition *g) { //@todo signature!
		int currentNodeId;
		MPI_Comm_rank(MPI_COMM_WORLD, &currentNodeId);
		int worldSize;
		MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

		MPI_Datatype* vertexMessage = VertexM::createVertexMessageDatatype(g->geGlobalVertexIdDatatype());

		this->result.first = new GlobalId[g->masterVerticesMaxCount()]();
		this->result.second = new GraphDist[g->masterVerticesMaxCount()];

		bool shouldContinue = true;
		std::vector<LocalId> frontier;

		/* append root to frontier if node matches */
		VERTEX_TYPE rootVt;
		auto rootLocal = g->toLocalId(this->bfsRoot, &rootVt);
		if(rootVt == L_SHADOW && rootVt == L_MASTER) {
			frontier.push_back(rootLocal);

			if(rootVt == L_MASTER) {
				this->result.first[rootLocal] = this->bfsRoot;
				this->result.second[rootLocal] = 0;
			}
		}

		auto sendBuffers = new VertexM[worldSize];
		auto outstandingSendRequests = new MPI_Request[worldSize];

		auto receiveBuffers = new VertexM[worldSize];
		auto outstandingReceiveRequests = new MPI_Request[worldSize];
		/* 2 below variables used for MPI_Testsome for both sending & receiving */
		int completed = 0;
		// this array is not really used while sending (but something must be passed)
		int *completedIndices = new int[worldSize];

		bool receivedAnything = false;
		auto othersReceivedAnything = new bool[worldSize];

		while(shouldContinue) {
			for(LocalId vid: frontier) {
				if(!g->isValid(this->getPredecessor(vid))) {
					/* node has not yet been visited */

					g->foreachNeighbouringVertex(vid, [&sendBuffers, vid, g, this](const GlobalId nid) {
						auto targetNode = g->toMasterNodeId(nid);
						VertexM *currBuffer = sendBuffers + targetNode;
						int currentId = currBuffer->vidCount;

						/* check if there is space left for yet another vertex */
						auto maxVertCount = details::fixedLen::MAX_VERTICES_IN_MESSAGE;
						if(currBuffer->vidCount >= maxVertCount) {
							/* @ToDo: rather ugly, can be fixed when internal iteration is removed */
							LOG(FATAL) << "Number of vertices in single send buffer (targetId: " << targetNode
							           << ") larger than allowed (" << maxVertCount << ")";
						} else {
							/* fill the data */
							currBuffer->vertexIds[currentId] = g->toLocalId(nid);
							currBuffer->predecessors[currentId] = g->toGlobalId(vid);
							currBuffer->distances[currentId] = this->getDistance(vid) + 1;
							currBuffer->vidCount += 1;
						}

						return true;
					});
				}
			}

			frontier.clear();

			/* initiate receive requests */
			for(int i = 0; i < worldSize; i++) {
				MPI_Irecv(receiveBuffers + i, 1, *vertexMessage, i, MPI_ANY_TAG, MPI_COMM_WORLD, outstandingReceiveRequests + i);
			}

			/* initiate send requests */
			for(int i = 0; i < worldSize; i++) {
				MPI_Isend(sendBuffers + i, 1, *vertexMessage, i, SEND_TAG, MPI_COMM_WORLD, outstandingSendRequests + i);
			}

			/* wait for all send requests to finish */
			completed = 0;
			while(completed < worldSize) {
				int completedInThisIt = 0;
				MPI_Testsome(worldSize, outstandingSendRequests, &completedInThisIt, completedIndices, MPI_STATUSES_IGNORE);
				completed += completedInThisIt;
			}

			/* wait for all (previously started) receive requests */
			completed = 0;
			while(completed < worldSize) {
				int completedInThisIt = 0;
				MPI_Testsome(worldSize, outstandingReceiveRequests, &completedInThisIt, completedIndices, MPI_STATUSES_IGNORE);

				/* iterate over all completed messages, processing received data */
				for(int i = 0; i < completedInThisIt; i++) {
					auto senderNodeId = completedIndices[i];
					auto currentBuffer = receiveBuffers[senderNodeId];

					/* iterate over all vertices in the message */
					for(int j = 0; j < currentBuffer.vidCount; j++) {
						/* save predecessor and distance for received node */
						this->getDistance(currentBuffer.vertexIds[j]) = currentBuffer.distances[j];
						this->getPredecessor(currentBuffer.vertexIds[j]) = currentBuffer.predecessors[j];

						/* add it to new frontier, which'll be processed during the next iteration */
						frontier.push_back(currentBuffer.vertexIds[j]);
					}
				}

				completed += completedInThisIt;
			}

			/* clear send buffers */
			for(int i = 0; i < worldSize; i++) {
				/* only vidCount needs to be reset, rest could be rubbish - no compression that I'm aware of,
				 * so skipping zeroing the rest should not be a problem */
				sendBuffers[i].vidCount = 0;
			}

			/* check if we can finished - we must ensure that no-one received any new nodes to process in this round */
			receivedAnything = frontier.size() > 0;
			othersReceivedAnything[currentNodeId] = receivedAnything;
			MPI_Allgather(&receivedAnything, 1, MPI_CXX_BOOL, othersReceivedAnything, 1, MPI_CXX_BOOL, MPI_COMM_WORLD);

			/* assume we cannot continue and find first node which receive something to process,
			 * thus proving assumption false */
			shouldContinue = false;
			for(int i = 0; i < worldSize && !shouldContinue; i++) {
				shouldContinue = othersReceivedAnything[i];
			}
		}

		/* ToDo - check if new returned memory */
		delete[] sendBuffers;
		delete[] outstandingSendRequests;
		delete[] completedIndices;
		delete[] othersReceivedAnything;
		VertexM::cleanupMpiDatatype(vertexMessage);

		return true;
	};
};


#endif //FRAMEWORK_BFSFIXEDMESSAGE_H
