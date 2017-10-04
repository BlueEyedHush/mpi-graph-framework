//
// Created by blueeyedhush on 18.07.17.
//

#include "Bfs.h"
#include <vector>
#include <stddef.h>
#include <mpi.h>
#include <glog/logging.h>
#include <utils/VariableLengthBufferManager.h>


std::pair<GlobalVertexId*, int*> *Bfs::getResult() {
	return &result;
}

Bfs::Bfs(GlobalVertexId _bfsRoot)
: result(nullptr, nullptr), bfsRoot(_bfsRoot) {

}

Bfs::~Bfs() {
	if(result.first != nullptr) delete[] result.first;
	if(result.second != nullptr) delete[] result.second;
}

/*
 * ****************************************************
 */

void Bfs_Mp_FixedMsgLen_1D_2CommRounds::createVertexMessageDatatype(MPI_Datatype *memory) {
	const int blocklens[] = {0, 1, MAX_VERTICES_IN_MESSAGE, MAX_VERTICES_IN_MESSAGE, MAX_VERTICES_IN_MESSAGE, 0};
	const MPI_Aint disparray[] = {
			0,
			offsetof(VertexMessage, vidCount),
			offsetof(VertexMessage, vertexIds),
			offsetof(VertexMessage, predecessors),
			offsetof(VertexMessage, distances),
	        sizeof(VertexMessage),
	};
	const MPI_Datatype types[] = {MPI_LB, MPI_INT, LOCAL_VERTEX_ID_MPI_TYPE,
	                              LOCAL_VERTEX_ID_MPI_TYPE, GRAPH_DIST_MPI_TYPE, MPI_UB};

	MPI_Type_create_struct(6, blocklens, disparray, types, memory);
	MPI_Type_commit(memory);
}

bool Bfs_Mp_FixedMsgLen_1D_2CommRounds::run(GraphPartition *g) {
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
	}

	auto sendBuffers = new VertexMessage[worldSize];
	auto outstandingSendRequests = new MPI_Request[worldSize];

	auto receiveBuffers = new VertexMessage[worldSize];
	auto outstandingReceiveRequests = new MPI_Request[worldSize];
	/* 2 below variables used for MPI_Testsome for both sending & receiving */
	int completed = 0;
	// this array is not really used while sending (but something must be passed)
	int *completedIndices = new int[worldSize];

	bool receivedAnything = false;
	auto othersReceivedAnything = new bool[worldSize];

	while(shouldContinue) {
		for(LocalVertexId vid: frontier) {
			if(!getPredecessor(vid).isValid()) {
				/* node has not yet been visited */

				g->forEachNeighbour(vid, [&sendBuffers, vid, this](GlobalVertexId nid) {
					auto targetNode = nid.nodeId;
					VertexMessage *currBuffer = sendBuffers + targetNode;
					int currentId = currBuffer->vidCount;

					/* check if there is space left for yet another vertex */
					if(currBuffer->vidCount >= MAX_VERTICES_IN_MESSAGE) {
						/* @ToDo: rather ugly, can be fixed when internal iteration is removed */
						LOG(FATAL) << "Number of vertices in single send buffer (targetId: " << targetNode
						           << ") larger than allowed (" << MAX_VERTICES_IN_MESSAGE << ")";
					} else {
						/* fill the data */
						currBuffer->vertexIds[currentId] = nid.localId;
						currBuffer->predecessors[currentId] = vid;
						currBuffer->distances[currentId] = this->getDistance(vid) + 1;
						currBuffer->vidCount += 1;
					}
				});
			}
		}

		frontier.clear();

		/* initiate receive requests */
		for(int i = 0; i < worldSize; i++) {
			MPI_Irecv(receiveBuffers + i, 1, vertexMessage, i, MPI_ANY_TAG, MPI_COMM_WORLD, outstandingReceiveRequests + i);
		}

		/* initiate send requests */
		for(int i = 0; i < worldSize; i++) {
			MPI_Isend(sendBuffers + i, 1, vertexMessage, i, SEND_TAG, MPI_COMM_WORLD, outstandingSendRequests + i);
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
					getDistance(currentBuffer.vertexIds[j]) = currentBuffer.distances[j];
					getPredecessor(currentBuffer.vertexIds[j]).nodeId = senderNodeId;
					getPredecessor(currentBuffer.vertexIds[j]).localId = currentBuffer.predecessors[j];

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

	return true;
}

Bfs_Mp_FixedMsgLen_1D_2CommRounds::Bfs_Mp_FixedMsgLen_1D_2CommRounds(GlobalVertexId _bfsRoot)
		: Bfs(_bfsRoot)
{

}

Bfs_Mp_FixedMsgLen_1D_2CommRounds::~Bfs_Mp_FixedMsgLen_1D_2CommRounds() {

}

/*
 * ****************************************************
 */

Bfs_Mp_VarMsgLen_1D_2CommRounds::Bfs_Mp_VarMsgLen_1D_2CommRounds(GlobalVertexId _bfsRoot) : Bfs(_bfsRoot) {

}

Bfs_Mp_VarMsgLen_1D_2CommRounds::~Bfs_Mp_VarMsgLen_1D_2CommRounds() {

}

void Bfs_Mp_VarMsgLen_1D_2CommRounds::createVertexMessageDatatype(MPI_Datatype *memory) {
	const int blocklens[] = {0, 1, 1, 1, 0};
	const MPI_Aint disparray[] = {
			0,
			offsetof(NewFrontierVertexInfo, vertexId),
			offsetof(NewFrontierVertexInfo, predecessor),
			offsetof(NewFrontierVertexInfo, distance),
			sizeof(NewFrontierVertexInfo),
	};
	const MPI_Datatype types[] = {MPI_LB, LOCAL_VERTEX_ID_MPI_TYPE,
	                              LOCAL_VERTEX_ID_MPI_TYPE, GRAPH_DIST_MPI_TYPE, MPI_UB};

	MPI_Type_create_struct(5, blocklens, disparray, types, memory);
	MPI_Type_commit(memory);
}

bool Bfs_Mp_VarMsgLen_1D_2CommRounds::run(GraphPartition *g) {
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

	return true;
}
