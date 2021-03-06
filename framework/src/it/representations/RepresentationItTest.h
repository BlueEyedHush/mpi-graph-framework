//
// Created by blueeyedhush on 25.04.18.
//

#ifndef FRAMEWORK_REPRESENTATIONITTEST_H
#define FRAMEWORK_REPRESENTATIONITTEST_H

#include <gtest/gtest.h>
#include <glog/logging.h>
#include <mpi.h>
#include <utils/TestUtils.h>
#include <bits/unordered_set.h>

typedef int TestLocalId;
typedef int TestNumId;
#define NUM_MPI_TYPE MPI_INT

class CommHelper {
public:
	CommHelper(size_t vCount,
	           MPI_Comm shmComm,
	           NodeId shmRank) : comm(shmComm), rank(shmRank)
	{
		/* allocate window */
		TestNumId *winData = nullptr;
		/* max amount of edges is vcount^2-1, but we also need first one for counting */
		// @todo: types might not fit! look at it! (add assert?)
		MPI_Win_allocate(2*vCount*vCount, sizeof(TestNumId), MPI_INFO_NULL, comm, &winData, &win);
		MPI_Win_lock_all(0, win);
	}

	~CommHelper() {
		MPI_Win_unlock_all(win);
		MPI_Win_free(&win);
	}

	/* writing */
	void appendEdge(TestNumId startV, TestNumId endV) {
		edgeDatas.push_back(startV);
		edgeDatas.push_back(endV);
	}

	void finishTransfers() {
		assert(edgeDatas.size() % 2 == 0);
		/* count must have same width as nods IDs - they are stored in the same window */
		TestNumId edgeCount = edgeDatas.size()/2;

		auto* data = edgeDatas.data();
		MPI_Put(&edgeCount, 1, NUM_MPI_TYPE, rank, 0, 1, NUM_MPI_TYPE, win);

		for(unsigned long i = 0; i < edgeCount; i++) {
			MPI_Put(data + 2*i, 1, NUM_MPI_TYPE, rank, 1 + 2*i, 1, NUM_MPI_TYPE, win);
			MPI_Put(data + 2*i + 1, 1, NUM_MPI_TYPE, rank, 1 + 2*i + 1, 1, NUM_MPI_TYPE, win);
		}

		MPI_Win_flush_all(win);
	}

	/* reading */
	std::set<std::pair<OriginalVertexId, OriginalVertexId>> getEdgesFrom(std::vector<OriginalVertexId>& originalVids,
	                                                                     std::vector<TestNumId>& mappedVids,
	                                                                     NodeId id) {
		/* here we use global ranks, but they should be the same */

		/* build mapping mapped -> original */
		assert(originalVids.size() == mappedVids.size());
		auto vCount = originalVids.size();

		std::unordered_map<TestNumId, OriginalVertexId> vmap;
		for(size_t i = 0; i < vCount; i++)
			vmap.emplace(mappedVids[i], originalVids[i]);


		TestNumId edgeCount = 0;
		MPI_Get(&edgeCount, 1, NUM_MPI_TYPE, id, 0, 1, NUM_MPI_TYPE, win);
		MPI_Win_flush(id, win);

		LOG(INFO) << "Edge count read from node " << id << ": " << edgeCount;

		auto *recvBuffer = new TestNumId[edgeCount*2];
		MPI_Get(recvBuffer, edgeCount*2, NUM_MPI_TYPE, id, 1, edgeCount*2, NUM_MPI_TYPE, win);
		MPI_Win_flush(id, win);

		std::set<std::pair<OriginalVertexId, OriginalVertexId>> edges;
		for(unsigned long i = 0; i < edgeCount; i++) {
			auto first = vmap[recvBuffer[2*i]];
			auto second = vmap[recvBuffer[2*i+1]];
			edges.emplace(std::make_pair(first, second));
		}

		delete[] recvBuffer;

		return edges;
	};

private:
	std::vector<TestNumId> edgeDatas;
	MPI_Comm comm;
	MPI_Win win;
	NodeId rank;
};

std::vector<OriginalVertexId> loadVertexListFromFile(std::string path);

std::set<std::pair<OriginalVertexId, OriginalVertexId>> loadEdgeListFromFile(std::string path);

template <typename TGraphHandle>
void representationTest(std::function<TGraphHandle(NodeId /*size*/,
                                                   NodeId /*rank*/,
                                                   std::vector<OriginalVertexId> /*idsToMap*/)> ghSupplier,
                        std::string datafilesPathPrefix) {
	int rank = -1;
	int size = -1;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	std::vector<OriginalVertexId> vertexIds = loadVertexListFromFile(datafilesPathPrefix + ".vl");
	auto expectedEdges = loadEdgeListFromFile(datafilesPathPrefix + ".el");

	auto builder = ghSupplier(size, rank, vertexIds);
	auto& gp = builder.getGraph();
	LOG(INFO) << "Loaded graph from file";

	CommHelper commHelper(vertexIds.size(), MPI_COMM_WORLD, rank);

	size_t masterEdgeCount = 0, masterVertexCount = 0;
	size_t shadowEdgeCount = 0, shadowVertexCount = 0;

	/* iterate masters & shadows, (source, target) pairs to file */
	LOG(INFO) << "Edges reported by representation: ";
	gp.foreachMasterVertex([&](const auto startLid) {
		gp.foreachNeighbouringVertex(startLid, [&](const auto endGid) {
			auto numFirst = gp.toNumeric(startLid);
			auto numSecond = gp.toNumeric(endGid);
			commHelper.appendEdge(numFirst, numSecond);
			masterEdgeCount += 1;
			LOG(INFO) << " M(" << numFirst << "," << numSecond << ")";
			return CONTINUE;
		});
		masterVertexCount += 1;
		return CONTINUE;
	});

	gp.foreachShadowVertex([&](auto startLid, auto startGid) {
		gp.foreachNeighbouringVertex(startLid, [&](auto endGid) {
			auto numFirst = gp.toNumeric(startLid);
			auto numSecond = gp.toNumeric(endGid);
			commHelper.appendEdge(numFirst, numSecond);
			shadowEdgeCount += 1;
			LOG(INFO) << " S(" << numFirst << "," << numSecond << ")";
			return CONTINUE;
		});
		shadowVertexCount += 1;
		return CONTINUE;
	});

	LOG(INFO) << "masterVertexCount: " << masterVertexCount << " masterEdgeCount: " << masterEdgeCount
	          << " shadowVertexCount: " << shadowVertexCount << " shadowEdgeCount: " << shadowEdgeCount;

	commHelper.finishTransfers();

	MPI_Barrier(MPI_COMM_WORLD);

	if (rank == 0) {
		std::vector<TestNumId> numIds;
		for(auto gid: builder.getConvertedVertices())
			numIds.push_back(gp.toNumeric(gid));

		/* gather edges */
		std::set<std::pair<OriginalVertexId, OriginalVertexId>> actualEdges;

		for(NodeId nodeId = 0; nodeId < size; nodeId++) {
			auto edgesFromNode = commHelper.getEdgesFrom(vertexIds, numIds, nodeId);
			LOG(INFO) << "Received " << edgesFromNode.size() << " from node " << nodeId;
			for(auto edge: edgesFromNode)
				LOG(INFO) << "  (" << edge.first << "," << edge.second << ")";
			actualEdges.insert(edgesFromNode.begin(), edgesFromNode.end());
		}

		/* compare against */
		ASSERT_EQ(actualEdges, expectedEdges);
	}
}

#endif //FRAMEWORK_REPRESENTATIONITTEST_H
