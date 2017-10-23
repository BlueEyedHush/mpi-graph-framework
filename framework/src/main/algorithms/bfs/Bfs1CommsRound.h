//
// Created by blueeyedhush on 19.10.17.
//

#ifndef FRAMEWORK_BFS1COMMSROUND_H
#define FRAMEWORK_BFS1COMMSROUND_H

#include <algorithms/Bfs.h>

template <class TGraphPartition>
class Bfs_Mp_VarMsgLen_1D_1CommsTag : public Bfs<TGraphPartition> {
private:
	IMPORT_ALIASES(TGraphPartition)
	using VertexM = details::varLength::VertexMessage<LocalId, GlobalId>;

public:
	const static int NOTHING_SENT_TAG = 1;
	const static int SOMETHING_SENT_TAG = 2;

	Bfs_Mp_VarMsgLen_1D_1CommsTag(const GlobalId _bfsRoot) : Bfs<TGraphPartition>(_bfsRoot) {};
	~Bfs_Mp_VarMsgLen_1D_1CommsTag() {};
	bool run(TGraphPartition *g) {
		int currentNodeId;
		MPI_Comm_rank(MPI_COMM_WORLD, &currentNodeId);
		int worldSize;
		MPI_Comm_size(MPI_COMM_WORLD, &worldSize);

		MPI_Datatype* vertexMessage = VertexM::createMpiDatatype(g->geGlobalVertexIdDatatype());

		this->result.first = new GlobalId[g->masterVerticesMaxCount()]();
		this->result.second = new int[g->masterVerticesMaxCount()];

		std::vector<LocalVertexId> frontier;

		/* append root to frontier if node matches */
		if(g->toMasterNodeId(this->bfsRoot) == currentNodeId) {
			auto lid = g->toLocalId(this->bfsRoot);
			frontier.push_back(lid);
			this->result.second[lid] = 0;
		}

		auto sendBuffers = new std::vector<VertexM>[worldSize];
		auto outstandingSendRequests = new MPI_Request[worldSize];
		int completed = 0;
		int* completedIndices = new int[worldSize];

		VariableLengthBufferManager<VertexM> receiveBufferManager;
		MPI_Status probeStatus;
		int elementCountInMessage;

		bool weSentAnything = false;
		bool anyoneSentAnything = true;

		while(anyoneSentAnything) {
			weSentAnything = false;
			anyoneSentAnything = false;

			for(LocalVertexId vid: frontier) {
				if(!g->isValid(this->getPredecessor(vid))) {
					/* node has not yet been visited */

					g->foreachNeighbouringVertex(vid, [&sendBuffers, &weSentAnything, vid, this, g](const GlobalId nid) {
						VertexM vInfo;
						vInfo.vertexId = nid.localId;
						vInfo.predecessor = g->toGlobalId(vid);
						vInfo.distance = this->getDistance(vid) + 1;
						sendBuffers[nid.nodeId].push_back(vInfo);
						weSentAnything = true;

						return true;
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
				          *vertexMessage,
				          i,
				          weSentAnything ? SOMETHING_SENT_TAG : NOTHING_SENT_TAG,
				          MPI_COMM_WORLD,
				          outstandingSendRequests + i);
			}

			anyoneSentAnything = anyoneSentAnything || weSentAnything;

			/* receive incoming messages and process data */
			for(completed = 0; completed < worldSize; completed++) {
				MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &probeStatus);
				MPI_Get_count(&probeStatus, *vertexMessage, &elementCountInMessage);
				anyoneSentAnything = anyoneSentAnything || (probeStatus.MPI_TAG == SOMETHING_SENT_TAG);
				auto senderId = probeStatus.MPI_SOURCE;
				VertexM *b = receiveBufferManager.getBuffer(static_cast<size_t>(elementCountInMessage));
				MPI_Recv(b, elementCountInMessage, *vertexMessage, senderId, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

				for(int i = 0; i < elementCountInMessage; i++) {
					VertexM *vInfo = b + i;

					/* save predecessor and distance for received node */
					this->getDistance(vInfo->vertexId) = vInfo->distance;
					this->getPredecessor(vInfo->vertexId) = vInfo->predecessor;

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
		}

		/* ToDo - check if new returned memory */
		delete[] sendBuffers;
		delete[] outstandingSendRequests;
		delete[] completedIndices;
		VertexM::cleanupMpiDatatype(vertexMessage);

		return true;
	};
};


#endif //FRAMEWORK_BFS1COMMSROUND_H
