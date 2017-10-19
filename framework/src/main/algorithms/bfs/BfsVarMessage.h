//
// Created by blueeyedhush on 19.10.17.
//

#ifndef FRAMEWORK_BFSVARMESSAGE_H
#define FRAMEWORK_BFSVARMESSAGE_H

#include <algorithms/Bfs.h>

template <typename TGraphPartition>
class Bfs_Mp_VarMsgLen_1D_2CommRounds : public Bfs<TestGP/*@todo*/> {
public:
	const static int SEND_TAG = 1;

	Bfs_Mp_VarMsgLen_1D_2CommRounds(const GlobalId _bfsRoot) : Bfs(_bfsRoot) {};

	virtual ~Bfs_Mp_VarMsgLen_1D_2CommRounds() {};

	virtual bool run(TGraphP *g) override {
		int currentNodeId;
		MPI_Comm_rank(MPI_COMM_WORLD, &currentNodeId);
		int worldSize;
		MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

		MPI_Datatype vertexMessage;
		createVertexMessageDatatype(&vertexMessage);

		result.first = new GlobalVertexId[g->getMaxLocalVertexCount()];
		result.second = new int[g->getMaxLocalVertexCount()];

		bool shouldContinue = true;
		std::vector<LocalVertexId> frontier;

		/* append root to frontier if node matches */
		if(bfsRoot.nodeId == currentNodeId) {
			frontier.push_back(bfsRoot.localId);
			result.second[bfsRoot.localId] = 0;
		}

		auto sendBuffers = new std::vector<NewFrontierVertexInfo>[worldSize];
		auto outstandingSendRequests = new MPI_Request[worldSize];
		int completed = 0;
		int* completedIndices = new int[worldSize];

		VariableLengthBufferManager<NewFrontierVertexInfo> receiveBufferManager;
		MPI_Status probeStatus;
		int elementCountInMessage;

		bool receivedAnything = false;
		auto othersReceivedAnything = new bool[worldSize];

		while(shouldContinue) {
			for(LocalVertexId vid: frontier) {
				if(!getPredecessor(vid).isValid()) {
					/* node has not yet been visited */

					g->forEachNeighbour(vid, [&sendBuffers, vid, this](GlobalVertexId nid) {
						NewFrontierVertexInfo vInfo;
						vInfo.vertexId = nid.localId;
						vInfo.predecessor = vid;
						vInfo.distance = this->getDistance(vid) + 1;
						sendBuffers[nid.nodeId].push_back(vInfo);
					});
				}
			}

			frontier.clear();

			/* According to standard this should not deadlock, but better keep an eye on it */

			/* initiate send requests */
			for(int i = 0; i < worldSize; i++) {
				auto& vec = sendBuffers[i];
				MPI_Isend(vec.data(),
				          static_cast<int>(vec.size()),
				          vertexMessage,
				          i,
				          SEND_TAG,
				          MPI_COMM_WORLD,
				          outstandingSendRequests + i);
			}

			/* receive incoming messages and process data */
			for(completed = 0; completed < worldSize; completed++) {
				MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &probeStatus);
				MPI_Get_count(&probeStatus, vertexMessage, &elementCountInMessage);
				auto senderId = probeStatus.MPI_SOURCE;
				NewFrontierVertexInfo *b = receiveBufferManager.getBuffer(static_cast<size_t>(elementCountInMessage));
				MPI_Recv(b, elementCountInMessage, vertexMessage, senderId, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				for(int i = 0; i < elementCountInMessage; i++) {
					NewFrontierVertexInfo *vInfo = b + i;

					/* save predecessor and distance for received node */
					getDistance(vInfo->vertexId) = vInfo->distance;
					getPredecessor(vInfo->vertexId).nodeId = senderId;
					getPredecessor(vInfo->vertexId).localId = vInfo->predecessor;

					/* add it to new frontier, which'll be processed during the next iteration */
					frontier.push_back(vInfo->vertexId);
				}
			}

			/* wait for all send requests to finish */
			completed = 0;
			while(completed < worldSize) {
				int completedInThisIt = 0;
				MPI_Testsome(worldSize, outstandingSendRequests, &completedInThisIt, completedIndices, MPI_STATUSES_IGNORE);
				completed += completedInThisIt;
			}

			/* clear send buffers */
			for(int i = 0; i < worldSize; i++) {
				sendBuffers[i].clear();
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
		/* cleanup type */

		return true;
	}
};


#endif //FRAMEWORK_BFSVARMESSAGE_H
