//
// Created by blueeyedhush on 26.05.17.
//

#ifndef FRAMEWORK_ADJACENCYLISTHASHPARTITION_H
#define FRAMEWORK_ADJACENCYLISTHASHPARTITION_H

#include <vector>
#include <sstream>
#include <boost/pool/object_pool.hpp>
#include <GraphBuilder.h>
#include <GraphPartition.h>
#include <utils/MpiTypemap.h>
#include <utils/AdjacencyListReader.h>



typedef unsigned long long ull;
const MPI_Datatype mpi_ull = MPI_UNSIGNED_LONG_LONG;

namespace details {
	const size_t FLUSH_EVERY = 10;

	struct PerNodeOffsetInfo {
		ull adjListOffset;
		ull vertexCount;
	};

	struct AdjListPos {
		AdjListPos(NodeId _nodeId, LocalVertexId _adjListOffset)
				: nodeId(_nodeId), adjListOffset(_adjListOffset) {}

		NodeId nodeId;
		LocalVertexId adjListOffset;
	};


	template<typename TLocalId, class TGlobalId>
	struct GraphData {
		MPI_Datatype gIdDatatype;

		int world_size;
		int world_rank;

		MPI_Win vertexEdgeWin, adjListWin, offsetTableWin;
		int offsetTableWinSize;
		int adjListWinSize;
		TLocalId *vertexEdgeWinMem;
		TLocalId *offsetTableWinMem;
		TGlobalId *adjListWinMem;
	};
}

template <typename TLocalId>
struct ALHPGlobalVertexId {
	ALHPGlobalVertexId() : ALHPGlobalVertexId(-1, 0) {}
	ALHPGlobalVertexId(NodeId nid, TLocalId lid) : nodeId(nid), localId(lid) {}

	NodeId nodeId;
	TLocalId localId;

	static MPI_Datatype mpiDatatype() {
		MPI_Datatype dt;
		int blocklengths[] = {1, 1};
		MPI_Aint displacements[] = {
				offsetof(ALHPGlobalVertexId, nodeId),
				offsetof(ALHPGlobalVertexId, localId)
		};
		MPI_Datatype building_types[] = {NODE_ID_MPI_TYPE, datatypeMap.at(typeid(TLocalId))};
		MPI_Type_create_struct(2, blocklengths, displacements, building_types, &dt);
		return dt;
	}
};

template <typename TLocalId, typename TNumId> class ALHPGraphBuilder;

template <typename TLocalId, typename TNumId> 
class ALHPGraphPartition : public GraphPartition<ALHPGlobalVertexId<TLocalId>, TLocalId, TNumId> {
private:
	using P = GraphPartition<ALHPGlobalVertexId<TLocalId>, TLocalId, TNumId>;
	IMPORT_ALIASES(P)
	using Gd = details::GraphData<LocalId, GlobalId>;

public:
	ALHPGraphPartition(Gd ds) : data(ds), vCount(ds.vertexEdgeWinMem[0]), eCount(ds.vertexEdgeWinMem[1]) {};
	ALHPGraphPartition(const ALHPGraphPartition&) = delete;
	ALHPGraphPartition& operator=(const ALHPGraphPartition&) = delete;
	ALHPGraphPartition(ALHPGraphPartition&& g) = default;
	ALHPGraphPartition& operator=(ALHPGraphPartition&& g) = default;

	MPI_Datatype geGlobalVertexIdDatatype() { return data.gIdDatatype; }

	LocalId toLocalId(const GlobalId gid, VERTEX_TYPE* vtype = nullptr) {
		if(vtype != nullptr) {
			*vtype = (gid.nodeId == data.world_rank) ? L_MASTER : NON_LOCAL;
		}

		return gid.localId;
	}

	NodeId toMasterNodeId(const GlobalId gid) {
		return gid.nodeId;
	}

	GlobalId toGlobalId(const LocalId lid) {
		return ALHPGlobalVertexId<LocalId>(data.world_rank, lid);
	}

	NumericId toNumeric(const GlobalId id) {
		unsigned int halfBitsInUll = (sizeof(unsigned long long)*CHAR_BIT)/2;
		unsigned long long numerical = ((unsigned long long) id.localId) << halfBitsInUll;
		numerical |= ((unsigned int) id.nodeId);
		return numerical;
	}

	NumericId toNumeric(const LocalId lid) {
		return toNumeric(ALHPGlobalVertexId<LocalId>(data.world_rank, lid));
	}

	std::string idToString(const GlobalId gid) {
		std::ostringstream os;
		os << "(" << gid.nodeId << "," << gid.localId << ")";
		return os.str();
	}

	std::string idToString(const LocalId lid) {
		std::ostringstream os;
		os << "(" << data.world_rank << "," << lid << ")";
		return os.str();
	}

	bool isSame(const GlobalId a, const GlobalId b) {
		return a.nodeId == b.nodeId && a.localId == b.localId;
	}

	bool isValid(const GlobalId id) {
		return id.nodeId >= 0;
	}


	void foreachMasterVertex(std::function<bool(const LocalId)>) {
		bool shouldStop = false;
		for(TLocalId vid = 0; vid < vCount && !shouldStop; vid++) {
			shouldStop = f(vid);
		}
	}

	size_t masterVerticesCount() {
		return vCount;
	}

	size_t masterVerticesMaxCount() {
		return data.offsetTableWinSize;
	}

	void foreachCoOwner(const LocalId lid, bool returnSelf, std::function<bool(const NodeId)> f) {
		if(returnSelf) {
			f(data.world_rank);
		}
	}

	void foreachNeighbouringVertex(const LocalId id, std::function<bool(const LocalId)> f) {
		auto startPos = data.offsetTableWinMem[id];
		auto endPos = (id < vCount-1) ? data.offsetTableWinMem[id+1] : eCount;

		bool shouldStop = false;
		for(LocalVertexId i = startPos; i < endPos && !shouldStop; i++) {
			GlobalId neighId = data.adjListWinMem[i];
			shouldStop = f(neighId);
		}
	};

	~ALHPGraphPartition() {}

private:
	Gd data;
	TLocalId vCount;
	TLocalId eCount;

	friend ALHPGraphBuilder<LocalId, NumericId>;
};

template <typename TLocalId, typename TNumId>
class ALHPGraphBuilder : public GraphBuilder<ALHPGraphPartition<TLocalId, TNumId>> {
private:
	using G = ALHPGraphPartition<TLocalId, TNumId>;
	IMPORT_ALIASES(G)
	using Gd = details::GraphData<LocalId, GlobalId>;

public:
	ALHPGraphBuilder() : convertedVertices(nullptr), convertedVerticesCount(0) {};

	G* buildGraph(std::string path, std::vector<OriginalVertexId> verticesToConvert) {
		/* rank 0 node partitions graph data across cluster in round-robin fashin
		 * other nodes are completly passive */
		using namespace details;

		int world_size;
		MPI_Comm_size(MPI_COMM_WORLD, &world_size);
		int world_rank;
		MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

		Gd d;
		MPI_Win vertexEdgeWin, adjListWin, offsetTableWin, convertedVerticesW;
		LocalId *vertexEdgeWinMem = nullptr;
		LocalId *offsetTableWinMem = nullptr;
		GlobalId *adjListWinMem = nullptr;

		d.gIdDatatype = ALHPGlobalVertexId<LocalId>::mpiDatatype();
		MPI_Type_commit(&d.gIdDatatype);

		ull sizes[2]; // 1st - edge list window size (adjList), 2nd - offset table window size

		AdjacencyListReader<LocalId> alReader(path);

		/* rank 0 reads graph file 'header' and then broadcasts sizes across cluster */
		if (world_rank == 0) {
			sizes[0] = alReader.getEdgeCount() / world_size + 1;
			sizes[1] = alReader.getVertexCount() / world_size + 1;
		}

		MPI_Bcast(sizes, 2, mpi_ull, 0, MPI_COMM_WORLD);

		MPI_Win_allocate(2*sizeof(LocalId), sizeof(LocalId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &vertexEdgeWinMem, &vertexEdgeWin);
		MPI_Win_allocate(sizes[0]*sizeof(GlobalId), sizeof(GlobalId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &adjListWinMem, &adjListWin);
		MPI_Win_allocate(sizes[1]*sizeof(LocalId), sizeof(LocalId), MPI_INFO_NULL, MPI_COMM_WORLD,
		                 &offsetTableWinMem, &offsetTableWin);

		MPI_Win_lock_all(0, vertexEdgeWin);
		MPI_Win_lock_all(0, adjListWin);
		MPI_Win_lock_all(0, offsetTableWin);

		auto vertToConvCount = verticesToConvert.size();
		if(vertToConvCount > 0) {
			convertedVertices = new ALHPGlobalVertexId<LocalId>[vertToConvCount];
			MPI_Win_create(convertedVertices,
			               vertToConvCount*sizeof(ALHPGlobalVertexId<LocalId>),
			               sizeof(ALHPGlobalVertexId<LocalId>),
			               MPI_INFO_NULL, MPI_COMM_WORLD,
			               &convertedVerticesW);
			MPI_Win_lock_all(0, convertedVerticesW);
		}

		if (world_rank == 0) {


			/* @todo: replace maps with simple arrays */

			/* when we read edge with end-vertex which hasn't yet been remapped, it goes here */
			std::unordered_map<ull, std::vector<AdjListPos>> toRemap;
			/* here we store information about vertices we already remapped (already remapped
			 * edges might be encountered in edges yet to come) */
			std::unordered_map<ull, GlobalId> remappingTable;
			/* tracks current insert position for each node @todo (why this is a map?) */
			std::unordered_map<NodeId, PerNodeOffsetInfo> nodeToOffsetInfo;
			for(int i = 0; i < world_size; i++) {
				nodeToOffsetInfo[i] = PerNodeOffsetInfo();
			}

			/* pools used for immediate RMA operations (1st for neighbour list, 2nd for offset table */
			boost::object_pool<ALHPGlobalVertexId<LocalId>> adjListPool;
			LocalId offsetPool[FLUSH_EVERY];

			bool allProcessed = false;
			NodeId nextNodeId = 0;
			/* this loop is executed until we reach end of file */
			while(!allProcessed) {
				/* below loop is for batching purposes (we flush only every FLUSH_EVERY) */
				for(int i = 0; i < FLUSH_EVERY && !allProcessed; i++) {

					auto vInfoOpt = alReader.getNextVertex();
					if(vInfoOpt) {
						VertexSpec<LocalId> vInfo = *vInfoOpt;

						size_t neighCount = vInfo.neighbours.size() - 1;
						LocalId adjListOffset = -1;

						/* we need to remap our vertex; due to round-robin nature of this algorithm
						 * it'll go to nextNodeId */
						ALHPGlobalVertexId<LocalId> vertexGid;
						vertexGid.nodeId = nextNodeId;

						/* 1st, lets read (and update) information about current offsets
						 * on target node */
						// @todo: make this a reference
						PerNodeOffsetInfo oinfo = nodeToOffsetInfo[vertexGid.nodeId];
						vertexGid.localId = oinfo.vertexCount++;
						adjListOffset = oinfo.adjListOffset;
						oinfo.adjListOffset += neighCount;
						nodeToOffsetInfo[vertexGid.nodeId] = oinfo;

						/* 2nd, register new mapping in remappingTable */
						remappingTable[vInfo.vertexId] = vertexGid;

						/* 3rd, choose node for next vertex */
						nextNodeId++;
						if(nextNodeId >= world_size) {
							nextNodeId = 0;
						}

						/* 4th prepare buffer which'll be used to update target's offset table */
						LocalId *offset = offsetPool + i;
						*offset = adjListOffset;

						/* 5th prepare buffer used to update neighbour list */
						ALHPGlobalVertexId<LocalId> *mappedNeigh =
								reinterpret_cast<ALHPGlobalVertexId<LocalId>*>(adjListPool.ordered_malloc(neighCount));

						/* 6th replace original edge end with remapped value (or postpone it until it's known) */
						// @todo: assuming vertices indices are continous,start from 0 and the file is sorted, we can guess the remapping
						size_t nextMappedNeighIndex = 0;
						for(auto it = vInfo.neighbours.begin(); it != vInfo.neighbours.end(); it++) {
							auto neighId = *it;
							if(remappingTable.count(neighId) > 0) {
								mappedNeigh[nextMappedNeighIndex] = remappingTable[neighId];
							} else {
								/* not yet mapped, we have to postpone it */
								if(toRemap.count(neighId) == 0) {
									toRemap[neighId] = std::vector<AdjListPos>();
								}
								toRemap[neighId].push_back(
										AdjListPos(vertexGid.nodeId, adjListOffset + nextMappedNeighIndex - 1));
								/* no need to write anything to mappedNeigh - it'll be overwritten later on */
							}

							nextMappedNeighIndex++;
						}

						/* 7th perform writes, one to offset table and one to edge list */
						MPI_Put(offset, 1, LOCAL_VERTEX_ID_MPI_TYPE,
						        vertexGid.nodeId, vertexGid.localId, 1, LOCAL_VERTEX_ID_MPI_TYPE,
						        offsetTableWin);
						MPI_Put(mappedNeigh, neighCount, d.gIdDatatype, vertexGid.nodeId,
						        adjListOffset, neighCount, d.gIdDatatype, adjListWin);


						/* finally, see if any of previously processed edges was connected to vertex we've just
						 * remapped. if the answer is yes, fill in the remapped value */

						// allocate buffer for MPI_Put containing single GlobalId (the one we are currently processing)
						ALHPGlobalVertexId<LocalId> *pooledMappedId = adjListPool.malloc();
						// @todo: use copy constructor if boost::pool allows it
						memcpy(pooledMappedId, &vertexGid, sizeof(ALHPGlobalVertexId<LocalId>));
						// initiate transfer to each place where placeholder was put
						for(AdjListPos coords: toRemap[vInfo.vertexId]) {
							MPI_Put(pooledMappedId, 1, d.gIdDatatype, coords.nodeId, coords.adjListOffset,
							        1, d.gIdDatatype, adjListWin);
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
			boost::pool<> LocalIdPool(sizeof(LocalId));
			for(int i = 0; i < world_size; i++) {
				LocalId *vCount = reinterpret_cast<LocalId*>(LocalIdPool.malloc());
				LocalId *eCount = reinterpret_cast<LocalId*>(LocalIdPool.malloc());

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
			LocalIdPool.purge_memory();

			/* save remapping info requested by user */
			for(size_t i = 0; i < verticesToConvert.size(); i++) {
				auto originalId = verticesToConvert[i];
				auto mappedId = remappingTable.at(originalId);

				// mirror this information across all nodes
				for(int nodeId = 0; nodeId < world_size; nodeId++) {
					MPI_Put(&mappedId, 1, d.gIdDatatype, nodeId, i, 1,
					        d.gIdDatatype, convertedVerticesW);
				}
			}

			MPI_Barrier(MPI_COMM_WORLD);
		} else {
			/* wait for master to partition vertices and edges across cluster */
			MPI_Barrier(MPI_COMM_WORLD);
		}

		if(vertToConvCount > 0) {
			MPI_Win_sync(convertedVerticesW);
			MPI_Win_unlock_all(convertedVerticesW);
			// window has been filled with data, no remote communication'll be required
			MPI_Win_free(&convertedVerticesW);
		}

		MPI_Win_sync(vertexEdgeWin);
		MPI_Win_sync(adjListWin);
		MPI_Win_sync(offsetTableWin);

		MPI_Win_unlock_all(vertexEdgeWin);
		MPI_Win_unlock_all(adjListWin);
		MPI_Win_unlock_all(offsetTableWin);
		/* adjacency list and offset list should be available */

		d.adjListWinSize = sizes[0];
		d.offsetTableWinSize = sizes[1];
		d.adjListWin = adjListWin;
		d.adjListWinMem = adjListWinMem;
		d.vertexEdgeWin = vertexEdgeWin;
		d.vertexEdgeWinMem = vertexEdgeWinMem;
		d.offsetTableWin = offsetTableWin;
		d.offsetTableWinMem = offsetTableWinMem;
		d.world_rank = world_rank;
		d.world_size = world_size;

		return new ALHPGraphPartition<LocalId, NumericId>(d);
	}

	std::vector<GlobalId> getConvertedVertices();

	void destroyGraph(const G* g) {
		MPI_Type_free(&(g->data.gIdDatatype));
		delete g;
	}

private:
	ALHPGlobalVertexId<LocalId>* convertedVertices;
	size_t convertedVerticesCount;
};

#endif //FRAMEWORK_ADJACENCYLISTHASHPARTITION_H
