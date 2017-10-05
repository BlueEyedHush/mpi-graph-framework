//
// Created by blueeyedhush on 26.05.17.
//

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cctype>
#include <cstddef>
#include <mpi.h>
#include <vector>
#include <unordered_map>
#include <new>
#include <boost/pool/pool.hpp>
#include "AdjacencyListHashPartition.h"

#define VERTEX_BUFFER_SIZE 10

typedef unsigned long long ull;
MPI_Datatype mpi_ull = MPI_UNSIGNED_LONG_LONG;

namespace {
	struct PerNodeOffsetInfo {
		ull adjListOffset;
		ull vertexCount;
	};

	struct AdjListPos {
		AdjListPos(NodeId _nodeId, LocalVertexId _adjListOffset) : nodeId(_nodeId), adjListOffset(_adjListOffset) {}

		NodeId nodeId;
		LocalVertexId adjListOffset;
	};
}

static std::vector<ull> parseLine(const std::string &line) {
	std::vector<ull> convertedLine;

	std::string num_str("");
	bool charInProgress = false;
	for(auto ch: line) {
		if(isdigit(ch)) {
			if (!charInProgress) {
				charInProgress = true;
			}
			num_str.push_back(ch);
		} else {
			if(charInProgress) {
				ull num = std::stoull(num_str);
				convertedLine.push_back(num);
				num_str.clear();
			}
		}
	}

	if(charInProgress && num_str.size() > 0) {
		ull num = std::stoull(num_str);
		convertedLine.push_back(num);
	}

	return convertedLine;
}

static void register_global_vertex_id(MPI_Datatype *dt) {
	int blocklengths[] = {1, 1};
	MPI_Aint displacements[] = {offsetof(GlobalVertexId, nodeId), offsetof(GlobalVertexId, localId)};
	MPI_Datatype building_types[] = {NODE_ID_MPI_TYPE, LOCAL_VERTEX_ID_MPI_TYPE};
	MPI_Type_create_struct(2, blocklengths, displacements, building_types, dt);

	MPI_Type_commit(dt);
}

GraphPartition* ALHPGraphBuilder::buildGraph(std::string path) {
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

	MPI_Win vertexEdgeWin, adjListWin, offsetTableWin;
	LocalVertexId *vertexEdgeWinMem = nullptr;
	LocalVertexId *offsetTableWinMem = nullptr;
	GlobalVertexId *adjListWinMem = nullptr;
	MPI_Datatype globalVertexDatatype;
	register_global_vertex_id(&globalVertexDatatype);

	ull adjListWinSize = 0;
	ull offsetTableWinSize = 0;

	if (world_rank == 0) {
		std::ifstream inputStream;
		inputStream.open(path, std::ifstream::in);
		std::string line;
		std::vector<ull> convertedLine;

		ull vertexCount;
		std::getline(inputStream, line);
		convertedLine = parseLine(line);
		vertexCount = *convertedLine.begin();

		ull edgeCount;
		std::getline(inputStream, line);
		convertedLine = parseLine(line);
		edgeCount = *convertedLine.begin();

		adjListWinSize = edgeCount/world_size + 1;
		offsetTableWinSize = vertexCount/world_size + 1;
		ull sizes[] = {adjListWinSize, offsetTableWinSize};

		MPI_Bcast(sizes, 2, mpi_ull, 0, MPI_COMM_WORLD);

		MPI_Win_allocate(2*sizeof(LocalVertexId), sizeof(LocalVertexId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &vertexEdgeWinMem, &vertexEdgeWin);
		MPI_Win_allocate(adjListWinSize*sizeof(GlobalVertexId), sizeof(GlobalVertexId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &adjListWinMem, &adjListWin);
		MPI_Win_allocate(offsetTableWinSize*sizeof(LocalVertexId), sizeof(LocalVertexId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &offsetTableWinMem, &offsetTableWin);

		MPI_Win_lock_all(0, vertexEdgeWin);
		MPI_Win_lock_all(0, adjListWin);
		MPI_Win_lock_all(0, offsetTableWin);

		/* read actual adjacency list and partition vertices across all machines */
		std::unordered_map<ull, std::vector<AdjListPos>> toRemap;
		std::unordered_map<ull, GlobalVertexId> remappingTable;
		std::unordered_map<NodeId, PerNodeOffsetInfo> nodeToOffsetInfo;
		for(int i = 0; i < world_size; i++) {
			nodeToOffsetInfo[i] = PerNodeOffsetInfo();
		}

		boost::pool<> adjListPool(sizeof(GlobalVertexId));
		LocalVertexId offsetPool[VERTEX_BUFFER_SIZE];

		bool allProcessed = false;
		NodeId nextNodeId = 0;
		while(!allProcessed) {
			for(int i = 0; i < VERTEX_BUFFER_SIZE && !allProcessed; i++) {
				line.clear();
				std::getline(inputStream, line);
				if (!line.empty()) {
					convertedLine = parseLine(line);

					ull vertexId = convertedLine[0];
					ull neighCount = convertedLine.size() - 1;
					LocalVertexId adjListOffset = -1;

					GlobalVertexId vertexGid;
					/* can't be remapped already, we need to remap */
					vertexGid.nodeId = nextNodeId;

					PerNodeOffsetInfo oinfo = nodeToOffsetInfo[vertexGid.nodeId];
					vertexGid.localId = oinfo.vertexCount++;
					adjListOffset = oinfo.adjListOffset;
					oinfo.adjListOffset += neighCount;
					nodeToOffsetInfo[vertexGid.nodeId] = oinfo;

					/* register mapping */
					remappingTable[vertexId] = vertexGid;

					nextNodeId++;
					if(nextNodeId >= world_size) {
						nextNodeId = 0;
					}

					/* prepare buffer with offset */
					LocalVertexId *offset = offsetPool + i;
					*offset = adjListOffset;

					/* now remap his neighbours */
					GlobalVertexId *mappedNeigh = reinterpret_cast<GlobalVertexId *>(adjListPool.ordered_malloc(neighCount));

					for(int neighIdx = 1; neighIdx < convertedLine.size(); neighIdx++) {
						ull neighId = convertedLine[neighIdx];
						if(remappingTable.count(neighId) > 0) {
							mappedNeigh[neighIdx-1] = remappingTable[neighId];
						} else {
							/* not yet mapped, we have to postpone it */
							if(toRemap.count(neighId) == 0) {
								toRemap[neighId] = std::vector<AdjListPos>();
							}
							toRemap[neighId].push_back(AdjListPos(vertexGid.nodeId, adjListOffset + neighIdx - 1));
							/* no need to write anything to mappedNeigh - it'll be overwritten later on */
						}
					}

					/* perform writes */
					MPI_Put(offset, 1, LOCAL_VERTEX_ID_MPI_TYPE,
					        vertexGid.nodeId, vertexGid.localId, 1, LOCAL_VERTEX_ID_MPI_TYPE,
					        offsetTableWin);
					MPI_Put(mappedNeigh, neighCount, globalVertexDatatype,
					        vertexGid.nodeId, adjListOffset, neighCount, globalVertexDatatype,
					        adjListWin);


					/* and finally, remap those that were already distributed, but replaced with placeholder */
					GlobalVertexId *pooledMappedId = reinterpret_cast<GlobalVertexId *>(adjListPool.malloc());
					memcpy(pooledMappedId, &vertexGid, sizeof(GlobalVertexId));
					for(AdjListPos coords: toRemap[vertexId]) {
						MPI_Put(pooledMappedId, 1, globalVertexDatatype,
						        coords.nodeId, coords.adjListOffset, 1, globalVertexDatatype,
						        adjListWin);
					}
				} else {
					allProcessed = true;
				}
			}

			MPI_Win_flush_local_all(offsetTableWin);
			MPI_Win_flush_local_all(adjListWin);
		}
		adjListPool.purge_memory();

		/* write number of vertices and edges for each node */
		boost::pool<> localVertexIdPool(sizeof(LocalVertexId));
		for(int i = 0; i < world_size; i++) {
			LocalVertexId *vCount = reinterpret_cast<LocalVertexId*>(localVertexIdPool.malloc());
			LocalVertexId *eCount = reinterpret_cast<LocalVertexId*>(localVertexIdPool.malloc());

			PerNodeOffsetInfo info = nodeToOffsetInfo[i];
			*vCount = info.vertexCount;
			*eCount = info.adjListOffset;

			MPI_Put(vCount, 1, LOCAL_VERTEX_ID_MPI_TYPE,
			        i, 0, 1, LOCAL_VERTEX_ID_MPI_TYPE,
			        vertexEdgeWin);
			MPI_Put(eCount, 1, LOCAL_VERTEX_ID_MPI_TYPE,
			        i, 1, 1, LOCAL_VERTEX_ID_MPI_TYPE,
			        vertexEdgeWin);
		}

		MPI_Win_flush_all(adjListWin);
		MPI_Win_flush_all(offsetTableWin);
		localVertexIdPool.purge_memory();

		MPI_Barrier(MPI_COMM_WORLD);

		MPI_Win_sync(adjListWin);
		MPI_Win_sync(offsetTableWin);

		MPI_Win_unlock_all(vertexEdgeWin);
		MPI_Win_unlock_all(adjListWin);
		MPI_Win_unlock_all(offsetTableWin);
		/* adjacency list and offset list should be available */

	} else {
		ull buffer[2];

		MPI_Bcast(buffer, 2, mpi_ull, 0, MPI_COMM_WORLD);
		adjListWinSize = buffer[0];
		offsetTableWinSize = buffer[1];

		MPI_Win_allocate(2*sizeof(LocalVertexId), sizeof(LocalVertexId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &vertexEdgeWinMem, &vertexEdgeWin);
		MPI_Win_allocate(adjListWinSize*sizeof(GlobalVertexId), sizeof(GlobalVertexId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &adjListWinMem, &adjListWin);
		MPI_Win_allocate(offsetTableWinSize*sizeof(LocalVertexId), sizeof(LocalVertexId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &offsetTableWinMem, &offsetTableWin);

		MPI_Win_lock_all(0, vertexEdgeWin);
		MPI_Win_lock_all(0, adjListWin);
		MPI_Win_lock_all(0, offsetTableWin);

		/* wait for master to load all data */
		MPI_Barrier(MPI_COMM_WORLD);

		MPI_Win_sync(vertexEdgeWin);
		MPI_Win_sync(adjListWin);
		MPI_Win_sync(offsetTableWin);

		MPI_Win_unlock_all(vertexEdgeWin);
		MPI_Win_unlock_all(adjListWin);
		MPI_Win_unlock_all(offsetTableWin);
		/* adjacency list and offset list should be available */

	}

	GraphData d;
	d.adjListWinSize = adjListWinSize;
	d.offsetTableWinSize = offsetTableWinSize;
	d.adjListWin = adjListWin;
	d.adjListWinMem = adjListWinMem;
	d.vertexEdgeWin = vertexEdgeWin;
	d.vertexEdgeWinMem = vertexEdgeWinMem;
	d.offsetTableWin = offsetTableWin;
	d.offsetTableWinMem = offsetTableWinMem;
	d.world_rank = world_rank;
	d.world_size = world_size;

	return new ALHPGraphPartition(d);
}




ALHPGraphPartition::ALHPGraphPartition(GraphData ds) : data(ds) {

}

int ALHPGraphPartition::getLocalVertexCount() {
	return data.vertexEdgeWinMem[0];
}

void ALHPGraphPartition::forEachNeighbour(LocalVertexId id, std::function<void(GlobalVertexId)> f) {
	LocalVertexId startPos = data.offsetTableWinMem[id];
	LocalVertexId endPos = (id < getLocalVertexCount()-1) ?
	                       data.offsetTableWinMem[id+1] :
	                       data.vertexEdgeWinMem[1];

	for(LocalVertexId i = startPos; i < endPos; i++) {
		GlobalVertexId neighId = data.adjListWinMem[i];
		f(neighId);
	}
}

void ALHPGraphPartition::forEachLocalVertex(std::function<void(LocalVertexId)> f) {
	for(LocalVertexId v_id = 0; v_id < getLocalVertexCount(); v_id++) {
		f(v_id);
	}
}

bool ALHPGraphPartition::isLocalVertex(GlobalVertexId id) {
	return id.nodeId == data.world_rank;
}

NodeId ALHPGraphPartition::getNodeId() {
	return data.world_rank;
}

unsigned long long ALHPGraphPartition::toNumerical(GlobalVertexId id) {
	unsigned int halfBitsInUll = (sizeof(unsigned long long)*CHAR_BIT)/2;
	unsigned long long numerical = ((unsigned long long) id.localId) << halfBitsInUll;
	numerical |= ((unsigned int) id.nodeId);
	return numerical;
}

ALHPGraphPartition::~ALHPGraphPartition() {
	MPI_Win_free(&data.vertexEdgeWin);
	MPI_Win_free(&data.offsetTableWin);
	MPI_Win_free(&data.adjListWin);
}

int ALHPGraphPartition::getMaxLocalVertexCount() {
	return data.adjListWinSize;
}
