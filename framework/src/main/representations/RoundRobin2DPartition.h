//
// Created by blueeyedhush on 23.11.17.
//

#ifndef FRAMEWORK_ROUNDROBIN2DPARTITION_H
#define FRAMEWORK_ROUNDROBIN2DPARTITION_H

#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <sstream>
#include <boost/pool/object_pool.hpp>
#include <GraphPartitionHandle.h>
#include <GraphPartition.h>
#include <utils/AdjacencyListReader.h>
#include <utils/NonCopyable.h>
#include <utils/MpiTypemap.h>
#include "shared.h"

template <typename TLocalId> using RR2DGlobalId = ALHPGlobalVertexId<TLocalId>;
template <typename TLocalId, typename TNumId> class RR2DHandle;

namespace details { namespace RR2D {
	using EdgeCount = unsigned int;
	using NodeCount = NodeId;
	using VertexHandle = unsigned int;
	using ElementCount = unsigned long long;
	const auto ElementCountDt = MPI_UNSIGNED_LONG_LONG;

	const ElementCount EDGES_MAX_COUNT = 1000;
	const ElementCount VERTEX_MAX_COUNT = 100;
	const ElementCount COOWNERS_MAX_COUNT = 20;

	struct EdgeTableOffset {
		EdgeTableOffset(NodeId nodeId, EdgeCount offset, bool master) : nodeId(nodeId), offset(offset), master(master) {}

		NodeId nodeId;
		EdgeCount offset;
		bool master;
	};

	template <typename T>
	class SendBufferManager : NonCopyable {
	public:
		SendBufferManager(NodeCount nodeCount) : nc(nodeCount) {
			buffers = new std::vector<T>[nc];
		}

		SendBufferManager(SendBufferManager&&) = default;
		SendBufferManager& operator=(SendBufferManager&&) = default;

		~SendBufferManager() {
			delete[] buffers;
		}

		void append(NodeId id, T value) {
			buffers[id].push_back(value);
		}

		void foreachNonEmpty(std::function<void(NodeId, T*, ElementCount)> f) {
			for(NodeId i = 0; i < nc; i++) {
				auto& b = buffers[i];
				if (!b.empty()) {
					f(i, b.data(), b.size());
				}
			}
		}

		void clearBuffers() {
			for(NodeId i = 0; i < nc; i++) {
				buffers[i].clear();
			}
		}

	private:
		NodeCount nc;
		std::vector<T> *buffers;
	};

	template <typename T>
	class MpiWindow : NonCopyable {
	public:
		MpiWindow(MPI_Datatype datatype, size_t size) : dt(datatype), size(size) {
			auto elSize = sizeof(T);
			MPI_Win_allocate(size*elSize, elSize, MPI_INFO_NULL, MPI_COMM_WORLD, &data, &win);
			MPI_Win_lock_all(0, win);
		};

		MpiWindow(MpiWindow&& o): dt(o.dt), win(o.win), data(o.data), size(o.size) {
			o.win = MPI_WIN_NULL;
			o.size = 0;
		};

		MpiWindow& operator=(MpiWindow&& o) {
			win = o.win;
			data = o.data;
			dt = o.dt;
			size = o.size;

			o.win = MPI_WIN_NULL;

			return *this;
		};

		~MpiWindow() {
			if (win != MPI_WIN_NULL) {
				MPI_Win_unlock_all(win);
				MPI_Win_free(&win);
			}
		}

		void put(NodeId nodeId, ElementCount offset, T* data, ElementCount dataLen) {
			MPI_Put(data + offset, dataLen, dt, nodeId, offset, dataLen, dt, win);
		}


		void flush() { MPI_Win_flush_all(win); }
		void sync() { MPI_Win_sync(win); }

		size_t getSize() {return size;}
		T* getData() { return data; }

	private:
		MPI_Win win;
		T* data;
		MPI_Datatype dt;
		size_t size;
	};

	struct OffsetArraySizeSpec {
		ElementCount valueCount;
		ElementCount offsetCount;

		static MPI_Datatype mpiDatatype() {
			MPI_Datatype dt;
			MPI_Type_contiguous(2, ElementCountDt, &dt);
			return dt;
		}
	};

	class OffsetTracker : NonCopyable {
	public:
		/* special members */
		OffsetTracker(ElementCount maxCount, NodeCount nodeCount)
				: nc(nodeCount),
				  offsets(new ElementCount[nc]),
				  maxCount(maxCount) {}

		OffsetTracker(OffsetTracker&& o) : maxCount(o.maxCount), offsets(o.offsets), nc(o.nc) {
			o.offsets = nullptr;
		}

		OffsetTracker& operator=(OffsetTracker&&) = delete;

		~OffsetTracker() {
			if(offsets != nullptr) delete[] offsets;
		}

		/* non-special members */

		ElementCount get(NodeId nid) {
			assert(nid < nc);
			return offsets[nid];
		}

		void tryAdvance(NodeId nid, ElementCount increment) {
			auto& offset = offsets[nid];
			assert(offset + increment < maxCount);
			offset += increment;
		}

	private:
		NodeCount nc;
		ElementCount* offsets;
		ElementCount maxCount;
	};

	template<typename T>
	class MpiWindowAppender : NonCopyable {
	public:
		MpiWindowAppender(MpiWindow<T>& win, NodeCount nodeCount)
				: window(win), bufferManager(SendBufferManager<T>(nodeCount)), offsetTracker(nodeCount, win.getSize()) {}

		void append(NodeId nid, T value) {
			bufferManager.append(nid, value);
		}

		void writeBuffers() {
			bufferManager.foreachNonEmpty([this](NodeId nid, T* buffer, ElementCount count) {
				auto currentWriteOffset = this->offsetTracker.get(nid);
				this->offsetTracker.tryAdvance(nid, count);
				this->window.put(nid, currentWriteOffset, buffer, count);
			});

			window.flush();
			bufferManager.clearBuffers();
		}
	private:
		MpiWindow<T>& window;
		SendBufferManager<T> bufferManager;
		OffsetTracker offsetTracker;
	};

	/* master must keep track of counts for each node separatelly */
	struct Counts {
		OffsetArraySizeSpec masters;
		OffsetArraySizeSpec shadows;
		/* offsetCount for coOwners should be identical to that of masters, but we keep it here for to keep design
		 * consistent */
		OffsetArraySizeSpec coOwners;
		ElementCount maxMastersCount;

		static MPI_Datatype mpiDatatype() {
			auto oascDt = OffsetArraySizeSpec::mpiDatatype();
			MPI_Datatype dt;
			int blocklengths[] = {1, 1, 1, 1};
			MPI_Aint displacements[] = {
					offsetof(Counts, masters),
					offsetof(Counts, shadows),
					offsetof(Counts, coOwners),
					offsetof(Counts, maxMastersCount)
			};
			MPI_Datatype building_types[] = {oascDt, oascDt, oascDt, ElementCountDt};
			MPI_Type_create_struct(4, blocklengths, displacements, building_types, &dt);
			return dt;
		}
	};

	class CountsForCluster : NonCopyable {
	public:
		CountsForCluster(NodeCount nc, MPI_Datatype countDt)
				: nc(nc),
				  counts(new Counts[nc]),
				  winDesc(MpiWindow<Counts>(1, countDt))
		{}

		CountsForCluster(CountsForCluster&& o) : nc(o.nc), counts(o.counts), winDesc(std::move(o.winDesc)) {
			assert(this != &o);
			o.counts = nullptr;
		}

		CountsForCluster& operator=(CountsForCluster&& o) {
			assert(this != &o);

			nc = o.nc;
			counts = o.counts;
			winDesc = std::move(o.winDesc);

			o.counts = nullptr;

			return *this;
		};

		~CountsForCluster() {
			if (counts != nullptr) delete[] counts;
		}

		/* meant to be used for both reading and _writing_ */
		Counts& get(NodeId nid) {
			return counts[nid];
		}

		void send() {
			for(NodeId nid = 0; nid < nc; nid++) {
				winDesc.put(nid, 0, counts + nid, 1);
			}
		}

		void flush() { winDesc.flush(); }
		void sync() { winDesc.sync(); }

	private:
		NodeCount nc;
		Counts *counts;
		MpiWindow<Counts> winDesc;
	};

	template <typename TLocalId>
	struct ShadowDescriptor {
		ShadowDescriptor(RR2DGlobalId<TLocalId> id, ElementCount offset) : edgeBeginningId(id), offset(offset) {}

		RR2DGlobalId<TLocalId> edgeBeginningId;
		ElementCount offset;

		static MPI_Datatype mpiDatatype(MPI_Datatype gidDt, MPI_Datatype elCountDt) {
			MPI_Datatype dt;
			int blocklengths[] = {1, 1};
			MPI_Aint displacements[] = {
					offsetof(ShadowDescriptor, edgeBeginningId),
					offsetof(ShadowDescriptor, offset)
			};
			MPI_Datatype building_types[] = {gidDt, elCountDt};
			MPI_Type_create_struct(2, blocklengths, displacements, building_types, &dt);
			return dt;
		}
	};

	struct MpiTypes {
		MPI_Datatype globalId;
		MPI_Datatype localId;
		MPI_Datatype shadowDescriptor;
		MPI_Datatype nodeId;
		MPI_Datatype count;
	};

	template <typename TLocalId>
	MpiTypes registerMpiTypes() {
		MpiTypes t;
		t.globalId = RR2DGlobalId<TLocalId>::mpiDatatype();
		MPI_Type_commit(&t.globalId);
		t.localId = getDatatypeFor<TLocalId>();
		t.nodeId = getDatatypeFor<NodeId>();
		t.count = getDatatypeFor<ElementCount>();
		t.shadowDescriptor = ShadowDescriptor<TLocalId>::mpiDatatype(t.globalId, t.count);
		MPI_Type_commit(&t.shadowDescriptor);

		return t;
	}

	/**
	 * t.globalId is not deregistered - after all algorithms are going to need it
	 * It could be cleaned up on graph release
	 * @param t
	 */
	void deregisterTypes(MpiTypes& t) {
		MPI_Type_free(&t.shadowDescriptor);
	}

	template <typename TLocalId, typename TNumId>
	struct GraphData {
		using GlobalId = RR2DGlobalId<TLocalId>;
		using ShadowDesc = ShadowDescriptor<TLocalId>;
		using LocalVerticesCount = TLocalId;

		GraphData(ElementCount mastersVsize, ElementCount mastersOsize,
		          ElementCount shadowsVsize, ElementCount shadowsOsize,
		          ElementCount coownersVsize, ElementCount coownersOsize,
		          ElementCount mappedIDsize, MpiTypes& dts)
				:
				mastersVwin(MpiWindow<GlobalId>(mastersVsize, dts.globalId)),
				mastersOwin(MpiWindow<TLocalId>(mastersOsize, dts.localId)),
                shadowsVwin(MpiWindow<GlobalId>(shadowsVsize, dts.globalId)),
                shadowsOwin(MpiWindow<ShadowDesc>(shadowsOsize, dts.shadowDescriptor)),
                coOwnersVwin(MpiWindow<NodeId>(coownersVsize, dts.nodeId)),
                coOwnersOwin(MpiWindow<NodeCount>(coownersOsize, dts.nodeId)),
                mappedIdsWin(MpiWindow<GlobalId>(mappedIDsize, dts.globalId))
		{}

		NodeId nodeId;
		NodeCount nodeCount;

		Counts counts;
		ElementCount remappedCount;
		ElementCount firstShadowId;

		MPI_Datatype globalIdDt;

		MpiWindow<GlobalId> mastersVwin;
		MpiWindow<LocalVerticesCount> mastersOwin;
		MpiWindow<NodeCount> coOwnersOwin;
		MpiWindow<NodeId> coOwnersVwin;

		MpiWindow<GlobalId> shadowsVwin;
		MpiWindow<ShadowDesc> shadowsOwin;
		std::unordered_map<TNumId, TLocalId> shadowsGlobalToLocalMap;

		/* this is used temporarily during building process and becomes invalid during normal operations */
		MpiWindow<GlobalId> mappedIdsWin;
	};

	/**
	 * Assumes we process one vertex at a time (start, register, finish)
	 * If we want to process more, we need some kind of VertexHandle to know which one we are talking about.
	 *
	 * After distributing data we don't expect any more communication. For efficiency, we let MPI allocate
	 * memory for windows, which means that lifetime of the window is tied to the lifetime of it's memory.
	 * This means that windows can be freed only after whole graph is released.
	 */
	template <typename TLocalId, typename TNumId>
	class CommunicationWrapper {
		using GlobalId = RR2DGlobalId<TLocalId>;
		using ShadowDesc = ShadowDescriptor<TLocalId>;
		/* using IDs for counts is rather unnatural (even if justified), so typedefing */
		using LocalVerticesCount = TLocalId;

	public:
		/* called by both master and slaves */
		CommunicationWrapper(GraphData<TLocalId, TNumId>& gd, MpiTypes& dts)
			: counts(CountsForCluster(gd.nodeCount, dts.count)),
			  gd(gd),

			  mastersV(gd.mastersVwin, gd.nodeCount),
			  mastersO(gd.mastersOwin, gd.nodeCount),
			  shadowsV(gd.shadowsVwin, gd.nodeCount),
			  shadowsO(gd.shadowsOwin, gd.nodeCount),
			  coOwnersV(gd.coOwnersVwin, gd.nodeCount),
			  coOwnersO(gd.coOwnersOwin, gd.nodeCount),

		      placeholdersPending(0)
		{}

		/* called by master */
		void finishAllTransfers() {
			gd.mastersVwin.flush();
			gd.mastersOwin.flush();
			gd.shadowsVwin.flush();
			gd.shadowsOwin.flush();
			gd.coOwnersVwin.flush();
			gd.coOwnersOwin.flush();
			counts.flush();

			placeholdersFreed();
		}

		/* called by slaves */
		void ensureTransfersVisibility() {
			gd.mastersVwin.sync();
			gd.mastersOwin.sync();
			gd.shadowsVwin.sync();
			gd.shadowsOwin.sync();
			gd.coOwnersVwin.sync();
			gd.coOwnersOwin.sync();
			counts.sync();
		}

		/* for sequential operation */
		void startAndAssignVertexTo(GlobalId gid) {
			currentVertexGid = gid;

			/* we need to add marker entries to masterNodeId's masters & coOwners offset tables so
			 * that node know that such vertex was assigned to it
			 * this is necessary even if there won't be any neighbour/coOwning nodeId assigned - in that case
			 * next in offset tables'll share the same ID
			 */

			auto& mastersCounts = counts.get(gid.nodeId).masters;
			/* put into offset table id of first unused cell in values table, then update offset table length */
			mastersO.append(gid.nodeId, mastersCounts.valueCount);
			mastersCounts.offsetCount += 1;

			auto& coOwnersCounts = counts.get(gid.nodeId).coOwners;
			/* same story as above */
			coOwnersO.append(gid.nodeId, coOwnersCounts.valueCount);
			/* in this case we skip updaing coOwnersCounts.offsetCounts since it must match mastersCounts.offsetCount */
		}

		void registerNeighbour(GlobalId neighbour, NodeId storeOn) {
			/*
			 * We need to:
			 * - check if storeOn is master; if not it needs to be added to coOwners (provided it's not yet there)
			 * - if it's a first shadow for this vertex assigned to storeOn, we have to create entry
			 * - if master: append entry to masters, otherwise: append entry to shadows (and update counts accordingly)
			 *  in shadows::offset table
			 */

			if(storeOn != currentVertexGid.nodeId) {
				currentVertexCoOwners.insert(storeOn);
				insertOffsetDescriptorOnShadowIfNeeded(storeOn);

				shadowsV.append(storeOn, neighbour);
				counts.get(storeOn).shadows.valueCount += 1;
			} else {
				mastersV.append(storeOn, neighbour);
				counts.get(storeOn).masters.valueCount += 1;
			}
		}

		void registerPlaceholderFor(OriginalVertexId oid, NodeId storeOn) {
			/*
			 * To lessen network utilization, we don't transfer placeholders, only reserve free space for them
			 * after actual data. This means we have to defer actual processing until all 'concrete' neighbours
			 * of current vertex are processed
			 */

			bool master = storeOn == currentVertexGid.nodeId;
			placeholders.push_back(std::make_pair(oid, EdgeTableOffset(storeOn, 0, master)));
		}

		/* returns offset under which placeholders were stored */
		std::vector<std::pair<OriginalVertexId, EdgeTableOffset>> finishVertex() {
			/*
			 * Frist we need to process placeholders - give them offsets and create offset entries for shadows
			 * if necessary
			 */
			for(auto& p: placeholders) {
				auto& desc = p.second;
				if (desc.master) {
					auto& c = counts.get(desc.nodeId).masters;
					desc.offset = c.valueCount;
					c.valueCount += 1;
				} else {
					currentVertexCoOwners.insert(desc.nodeId);
					insertOffsetDescriptorOnShadowIfNeeded(desc.nodeId);

					auto& c = counts.get(desc.nodeId).shadows;
					desc.offset = c.valueCount;
					c.valueCount += 1;
				}
			}

			/* write information about coowners */
			coOwnersO.append(currentVertexGid.nodeId, counts.get(currentVertexGid.nodeId).coOwners.valueCount);
			for(auto& coowningNode: currentVertexCoOwners) {
				coOwnersV.append(currentVertexGid.nodeId, coowningNode);
				counts.get(currentVertexGid.nodeId).coOwners.valueCount += 1;
			}

			/* send stuff */
			mastersV.writeBuffers();
			mastersO.writeBuffers();
			shadowsV.writeBuffers();
			shadowsO.writeBuffers();
			coOwnersV.writeBuffers();
			coOwnersO.writeBuffers();
			placeholdersFreed();

			/* cleanup */
			currentVertexCoOwners.clear();
			auto placehodersRet(placeholders);
			placeholders.clear();

			return placehodersRet;
		};

		/* for placeholder replacement */
		void replacePlaceholder(EdgeTableOffset eto, GlobalId gid) {
			auto* buffer = placeholderReplacementBuffers.malloc();
			*buffer = gid;

			auto& oa = eto.master ? gd.mastersVwin : gd.mastersOwin;
			oa.put(eto.nodeId, eto.offset, buffer, 1);

			placeholdersPending += 1;
			/*
			 * Thanks to the fact that we don't actually write placeholders, we don't need flush() between
			 * writing placeholder and writing actual value
			 */

			/* However, if the buffer grew too big, we want to flush it and release memory */
			if (placeholdersPending > maxPlaceholdersPending) {
				gd.mastersVwin.flush();
				gd.shadowsOwin.flush();
				placeholdersFreed();
			}
		}

		void setMaxMasterCount(NodeId targetNode, ElementCount maxCount) {
			counts.get(targetNode).maxMastersCount = maxCount;
		}

		const Counts& getCountFor(NodeId nodeId) { return counts.get(nodeId); };

	private:
		void insertOffsetDescriptorOnShadowIfNeeded(NodeId storeOn) {
			auto& c = counts.get(storeOn).shadows;
			if(currentVertexCoOwners.count(storeOn) == 0) {
				shadowsO.append(storeOn, ShadowDesc(currentVertexGid, c.valueCount));
				c.offsetCount += 1;
			}
		}

		void placeholdersFreed() {
			placeholderReplacementBuffers.release_memory();
			placeholdersPending = 0;
		}

	private:
		const static size_t maxPlaceholdersPending = 100;
		size_t placeholdersPending;

		/* 'global' members, shared across all vertices and nodes */
		CountsForCluster counts;
		GraphData<TLocalId, TNumId>& gd;

		MpiWindowAppender<GlobalId> mastersV;
		MpiWindowAppender<LocalVerticesCount> mastersO;
		MpiWindowAppender<GlobalId> shadowsV;
		MpiWindowAppender<ShadowDesc> shadowsO;
		MpiWindowAppender<NodeId> coOwnersV;
		MpiWindowAppender<NodeCount> coOwnersO;

		/* per-vertex members, reset when new vertex is started */
		GlobalId currentVertexGid;
		std::unordered_set<NodeId> currentVertexCoOwners;
		std::vector<std::pair<OriginalVertexId, EdgeTableOffset>> placeholders;
		boost::object_pool<GlobalId> placeholderReplacementBuffers;
	};

	template <typename TLocalId>
	class RemappingTable {
		using GlobalId = RR2DGlobalId<TLocalId>;

	public:
		void registerMapping(OriginalVertexId oid, GlobalId gid) {
			remappingTable.emplace(oid, gid);
		}

		boost::optional<GlobalId> toGlobalId(OriginalVertexId oid) {
			const auto fresult = remappingTable.find(oid);
			return (fresult != remappingTable.end()) ? (*fresult).second : boost::none;
		}

		void releaseMapping(OriginalVertexId oid) {
			remappingTable.erase(oid);
		}

	private:
		std::unordered_map<OriginalVertexId, GlobalId> remappingTable;
	};

	class PlaceholderCache {
	public:
		/* access pattern - after remapping vertex we want to replace all occurences */
		void rememberPlaceholder(OriginalVertexId oid, EdgeTableOffset eto) {
			/* exploiting the fact that, when the key is absent, map::operator[] inserts key with
			 * default-constructed value */
			cache[oid].push_back(eto);
		}

		void foreachPlaceholderFor(OriginalVertexId oid, std::function<void(EdgeTableOffset)> f) {
			const auto fresult = cache.find(oid);
			if(fresult != cache.end()) {
				for(auto eto: (*fresult).second) {
					f(eto);
				}
			}
		}

	private:
		std::unordered_map<OriginalVertexId, std::vector<EdgeTableOffset>> cache;
	};

	template <typename TLocalId>
	class Partitioner {
	public:
		Partitioner(NodeCount nodeCount)
				: nodeCount(nodeCount), nextMasterNodeId(0), nextLocalId(new TLocalId[nodeCount]),
				  nextNeighbourNodeId(0), largetstAssignedLocalId(0)
		{}

		~Partitioner() {
			if(nextLocalId != nullptr) delete[] nextLocalId;
		}

		RR2DGlobalId<TLocalId> nextMasterId() {
			NodeId nid = nextMasterNodeId;
			nextMasterNodeId += 1;

			TLocalId& nextLid = nextLocalId[nid];
			TLocalId lid = nextLid;
			nextLid += 1;

			if (nextLid > largetstAssignedLocalId)
				largetstAssignedLocalId = nextLid;

			/* assuming that user makes series of calls to nextNodeIdForNeighbour right after calling nextMasterId,
			 * starting neighbourNodeIds with masterNodeId guarnatees that master won't be empty (which'd be legal, but
			 * a waste)
			 */
			nextNeighbourNodeId = nid;

			return RR2DGlobalId<TLocalId>(nid, lid);
		}

		NodeId nextNodeIdForNeighbour() {
			return nextNeighbourNodeId++;
		}

		TLocalId getLargestAssignedLocalId() {return largetstAssignedLocalId;}

	private:
		NodeCount nodeCount;
		NodeId nextMasterNodeId;
		TLocalId *nextLocalId;
		NodeId nextNeighbourNodeId;
		TLocalId largetstAssignedLocalId;
	};

	template<typename TLocalId, typename TNumId>
	void handleRemapping(GraphData<TLocalId, TNumId> *gd,
	                     std::vector<OriginalVertexId> verticesToConvert,
	                     RemappingTable<TLocalId> rt) {
		MpiWindowAppender<RR2DGlobalId<TLocalId>> remappingWinAppender(gd->mappedIdsWin, gd->nodeCount);
		for(auto oId: verticesToConvert) {
			for(NodeId nid = 0; nid < gd->nodeCount; nid++) {
				auto correspondingGid = *rt.toGlobalId(oId);
				remappingWinAppender.append(nid, correspondingGid);
			}
		}
		remappingWinAppender.writeBuffers();
		gd->mappedIdsWin.flush();
	}

	template<typename TLocalId, typename TNumId>
	std::vector<RR2DGlobalId<TLocalId>> extractRemapedVerticesToVector(GraphData<TLocalId, TNumId> *gd) {
		std::vector<RR2DGlobalId<TLocalId>> remappedVertices(gd->remappedCount);
		auto mappedData = gd->mappedIdsWin.getData();

		for(ElementCount i = 0; i < gd->remappedCount; i++) {
			remappedVertices.push_back(mappedData[i]);
		}

		return remappedVertices;
	}
} }

template <typename TLocalId, typename TNumId>
class RoundRobin2DPartition : public GraphPartition<RR2DGlobalId<TLocalId>, TLocalId, TNumId> {
	using P = GraphPartition<RR2DGlobalId<TLocalId>, TLocalId, TNumId>;
	IMPORT_ALIASES(P)

	RoundRobin2DPartition(details::RR2D::GraphData<LocalId, NumericId> *graphData) : graphData(graphData) {}

public:
	MPI_Datatype getGlobalVertexIdDatatype() { return graphData->globalIdDt; }

	LocalId toLocalId(const GlobalId gid, VERTEX_TYPE* vtype = nullptr) {
		auto& map = graphData->shadowsGlobalToLocalMap;

		if (gid.nodeId == graphData->nodeId) {
			if (vtype != nullptr) *vtype = L_MASTER;
			return gid.localId;
		} else  {
			auto it = map.find(toNumeric(gid));
			if (it != map.end()) {
				if (vtype != nullptr) *vtype = L_SHADOW;
				return it->second;
			} else {
				if (vtype != nullptr) *vtype = NON_LOCAL;
				return 0;
			}
		}
	}

	NodeId toMasterNodeId(const GlobalId gid) {
		return gid.nodeId;
	}

	GlobalId toGlobalId(const LocalId lid) {
		const auto masterCount = graphData->counts.masters.offsetCount;
		const auto shadowCount = graphData->counts.shadows.offsetCount;

		if (lid >= 0 && lid < masterCount) {
			/* we deal with master, just construct it? */
			return GlobalId(graphData->nodeId, lid);
		} else if (lid >= graphData->firstShadowId && lid < graphData->firstShadowId + shadowCount) {
			/* when dealing with shadows, we have look into ShadowDesc window */
			const auto relativeShadowId = lid - graphData->firstShadowId;
			return graphData->shadowsOwin.getData()[relativeShadowId].edgeBeginningId;
		} else {
			/* just throw exception */
			throw std::string("LocalID out of range");
		}
	}

	NumericId toNumeric(const GlobalId gid) {
		return globalToNumericId<GlobalId, NumericId>(gid);
	}

	NumericId toNumeric(const LocalId lid) {
		return toNumeric(GlobalId(graphData->nodeId, lid));
	}

	std::string idToString(const GlobalId gid) {
		std::ostringstream os;
		os << "(" << gid.nodeId << "," << gid.localId << ")";
		return os.str();
	}

	std::string idToString(const LocalId lid) {
		std::ostringstream os;
		os << "(" << graphData->nodeId << "," << lid << ")";
		return os.str();
	}

	bool isSame(const GlobalId a, const GlobalId b) {
		return a.nodeId == b.nodeId && a.localId == b.localId;
	}

	bool isValid(const GlobalId gid) {
		return gid.nodeId >= 0;
	}


	void foreachMasterVertex(std::function<ITER_PROGRESS (const LocalId)> f) {
		ITER_PROGRESS ip = CONTINUE;
		for(TLocalId vid = 0; vid < graphData->counts.masters.offsetCount && ip == CONTINUE; vid++) {
			ip = f(vid);
		}
	}

	size_t masterVerticesCount() {
		return graphData->counts.masters.offsetCount;
	}

	size_t masterVerticesMaxCount() {
		return graphData->counts.maxMastersCount;
	}


	void foreachShadowVertex(std::function<ITER_PROGRESS (const LocalId, const GlobalId)> f) {
		ITER_PROGRESS ip = CONTINUE;
		for(TLocalId lid = 0; lid < graphData->counts.shadows.offsetCount && ip == CONTINUE; lid++) {
			auto gid = graphData->shadowsOwin.getData()[lid].edgeBeginningId;
			ip = f(lid + graphData->firstShadowId, gid);
		}
	}

	size_t shadowVerticesCount() {
		return graphData->counts.shadows.offsetCount;
	}


	void foreachCoOwner(LocalId localId, bool returnSelf, std::function<ITER_PROGRESS (const NodeId)> f) {
		// @todo: range check
		auto coownerIdx = graphData->coOwnersOwin.getData()[localId];
		auto limit = (localId == graphData->counts.coOwners.offsetCount) ?
	                 graphData->counts.coOwners.valueCount :
	                 graphData->coOwnersOwin.getData()[localId+1]; // @todo: cast needed here

		while(coownerIdx < limit) {
			auto coownerId = graphData->coOwnersVwin.getData()[coownerIdx];
			f(coownerId);
			coownerIdx += 1;
		}

		/* owner is not writtn to coowners window */
		if (returnSelf) f(graphData->nodeId);
	}

	void foreachNeighbouringVertex(LocalId id, std::function<ITER_PROGRESS (const GlobalId)> f) {
		size_t startPos, endPos;
		details::RR2D::MpiWindow<GlobalId>* winWithValues;
		if (id >= graphData->firstShadowId) {
			/* shadow */
			auto relativeId = id - graphData->firstShadowId;
			startPos = graphData->shadowsOwin.getData()[relativeId].offset;
			endPos = (id < graphData->counts.masters.offsetCount-1) ?
			         graphData->shadowsOwin.getData()[relativeId+1].offset :
			         graphData->counts.masters.valueCount;

			winWithValues = &(graphData->shadowsVwin);
		} else {
			/* master */
			startPos = graphData->mastersOwin.getData()[id];
			endPos = (id < graphData->counts.masters.offsetCount-1) ?
			         graphData->mastersOwin.getData()[id+1] :
			         graphData->counts.masters.valueCount;

			winWithValues = &(graphData->mastersVwin);
		}

		ITER_PROGRESS ip = CONTINUE;
		for(LocalVertexId i = startPos; i < endPos && ip == CONTINUE; i++) {
			GlobalId neighId = winWithValues->getData()[i];
			ip = f(neighId);
		}
	}

private:
	friend class RR2DHandle<LocalId, NumericId>;

	details::RR2D::GraphData<LocalId, NumericId> *graphData;
};



/**
 * This class is a handle to a graph data, but it's main purpose is loading and partitioning of the graph.
 * High-level overview of the process:
 * - only master reads file and distributes data on other nodes using RMA
 * - when master reads vertex and it's neighbours from file, it chooses (in round-robin manner) to which
 * 	node this vertex should be assign (that node becomes it's master node)
 * - then it generates for vertex GlobalId, consisting of NodeId and LocalId
 * - he also saves mapping between OriginalVertexId and GlobalId (so that it knows how to remap edges that
 * 	this vertex is connected by)
 * - We may encouter and edge whose end has not yet been remapped. In that case we save information about that in
 * 	separate datastructure, and write to that node a placeholder value.
 * - for each vertex we process, we split neighbours into chunks which are assigned to different nodes
 * - we write to master (for our vertex) information about all nodes which store it's neighbours
 * - master vertices and shadows are written to different windows due to requirement that shadows must be stored after
 * 	masters, and we don't know how many vertices are going to be assigned to given node.
 * - nodes must have GlobalId -> LocalId mapping. This is easy for masters since their GlobalId = (nodeId, localId). But
 * 	for shadows we need to know original GlobalId - it must be distributed to the node, alongside assigned neighbours.
 * 	Therefore, for shadows, value table consists only from neighbours (just like for masters), but each position in
 * 	offset table contains pair (offset, originalGlobalId)
 * - after distribution is finished, ma]ster discards GlobalId -> LocalId mapping, except from values, that were
 * 	user requested to be kept (verticesToConv constructor parameter)
 *
 * 	Datastructures needed:
 * 	- Win: edge data - masters
 * 	- Win: offset table - masters
 * 	- Win: edge data - shadows
 * 	- Win: index (offset + corresponding GlobalId) - shadows
 * 	- Win: counts - master vertices, master edges, shadow vertices, shadow edges, owners table size
 * 	- Win: GlobalId -> LocalId mapping for vertices for which it has been requested
 * 	- Win: NodeIds - where are stored neighbours (list of nodes)
 * 	- Win: offsets for the above table
 * 	- master: GlobalId -> LocalId map
 * 	- master: NodeId -> (OriginalVertexId, offset) map - placeholders that need replacing
 *
 * Sizes of the windows:
 * we can try to estimate it, but knowing for sure would require vertex count and edge count. At one point
 * some resizing scheme probably needs to be developed, but for now let's keep it configurable and throw error if
 * it is not enough.
 *
 * Components:
 * - Parser - loads data from file. How does it manage memory?
 * - CommunicationWrapper
 * 	- initializes all windows,
 * 	- keeps track of the offsets,
 * 	- gives appending semantics (saving to window, returns offset) and random access
 * 	 (for fixing placeholders)
 * 	- batches provided updates and sends them
 * 	- also wraps flushing
 * 	- and setting on master node for vertex where are neighbours stored
 * - RemappingTable - holds OriginalVertexId -> GlobalId data. Even if we remap immediatelly, we still needs this mapping
 * 	for following edges we load.
 * - PlaceholderCache - holds information about holes that must be filled after we remap
 * - Partitioner - which vertex should go to whom. Could be coupled wth GlobalId generator?

 * Algorithm:
 * - data read from file by Parser, which returns vector containing whole lines
 * - GlobalId for processed vertex is not really stored, but neighbours are - we retrieve them from
 * 	Partitioner
 * 	- even though it's not stored, it must be created - we assign node as a master on an round-robin
 * 	 manner, independently from edges (which means we might end up with pretty stupid partitioning
 * 	 where master doesn't store any edges)
 * - we read mapping for neighbours from RemappingTable (or use placeholder) and submit it to CommunicationWrapper
 * 	- It can reorder vertices so that placeholders are at the end of the list (and don't really have
 * 	  to be transfered)
 * - after we finish with current vertex, CommunicationWrapper is ready to send message
 * - we also need to replace placeholders
 * 	- wait at the end of processing and then replace all
 * 	- replace as soon as mapping becomes available - helps reduce size of the mapping
 *
 */

template <typename TLocalId, typename TNumId>
class RR2DHandle : public GraphPartitionHandle<RoundRobin2DPartition<TLocalId, TNumId>> {
	using G = RoundRobin2DPartition<TLocalId, TNumId>;
	using P = GraphPartitionHandle<G>;
	IMPORT_ALIASES(G)

public:
	RR2DHandle(std::string path, std::vector<OriginalVertexId> verticesToConv)
			: P(verticesToConv, destroyGraph), path(path)
	{

	}

protected:
	virtual std::pair<G*, std::vector<GlobalId>>
	buildGraph(std::vector<OriginalVertexId> verticesToConvert) override {
		using namespace details::RR2D;

		int nodeCount, nodeId;
		MPI_Comm_size(MPI_COMM_WORLD, &nodeCount);
		MPI_Comm_rank(MPI_COMM_WORLD, &nodeId);

		MpiTypes types = registerMpiTypes<LocalId>();

		auto gd = new details::RR2D::GraphData<LocalId, NumericId>(EDGES_MAX_COUNT,
		                                                           VERTEX_MAX_COUNT,
		                                                           EDGES_MAX_COUNT,
		                                                           VERTEX_MAX_COUNT,
		                                                           COOWNERS_MAX_COUNT*VERTEX_MAX_COUNT,
		                                                           COOWNERS_MAX_COUNT,
		                                                           verticesToConvert.size(),
		                                                           types);
		gd->nodeId = nodeId;
		gd->nodeCount = nodeCount;
		gd->remappedCount = verticesToConvert.size();
		gd->globalIdDt = types.globalId;

		CommunicationWrapper<LocalId, NumericId> cm(*gd, types);

		if (nodeId == 0) {
			AdjacencyListReader<OriginalVertexId> reader(path);
			Partitioner<LocalId> partitioner(nodeCount);
			RemappingTable<LocalId> remappingTable;
			PlaceholderCache placeholderCache;

			while(auto optionalVertexSpec = reader.getNextVertex()) {
				auto vspec = optionalVertexSpec.get();

				/* get mapping and register it */
				GlobalId mappedId = partitioner.nextMasterId();
				remappingTable.registerMapping(vspec.vertexId, mappedId);

				/* remap neighbours we can (or use placeholders) and distribute to target nodes */
				cm.startAndAssignVertexTo(mappedId);
				for(auto neighbour: vspec.neighbours) {
					NodeId nodeIdForNeigh = partitioner.nextNodeIdForNeighbour();

					if (auto optionalGid = remappingTable.toGlobalId(neighbour)) {
						cm.registerNeighbour(optionalGid.get(), nodeIdForNeigh);
					} else {
						cm.registerPlaceholderFor(neighbour, nodeIdForNeigh);
					}
				}
				auto placeholders = cm.finishVertex();

				/* remember placeholders for further replacement */
				for(auto p: placeholders) {
					auto originalId = p.first;
					auto offset = p.second;

					placeholderCache.rememberPlaceholder(originalId, offset);
				}

				/* remap placeholders that refered to node we just remapped */
				placeholderCache.foreachPlaceholderFor(vspec.vertexId, [&cm, &mappedId](EdgeTableOffset eto) {
					cm.replacePlaceholder(eto, mappedId);
				});
			}

			/* update maxLocalId count */
			const auto mmc = partitioner.getLargestAssignedLocalId() + 1;
			for(NodeCount nid = 0; nid < nodeCount; nid++)
				cm.setMaxMasterCount(nid, mmc);

			cm.finishAllTransfers();

			/* save requested remapping info */
			handleRemapping(gd, verticesToConvert, remappingTable);

			/* signal other nodes that graph data distribution has been finisheds */
			MPI_Barrier(MPI_COMM_WORLD);

		} else {
			/* let master do her stuff */
			MPI_Barrier(MPI_COMM_WORLD);

			/* ensure all transfer from masters are visible */
			cm.ensureTransfersVisibility();
			gd->mappedIdsWin.sync();
		}

		deregisterTypes(types);
		/* now each node must use contents of shadow-related windows to build GlobalId -> LocalId map */
		LocalId nextShadowLocalId = gd->counts.maxMastersCount;
		gd->firstShadowId = nextShadowLocalId;
		for(ElementCount i = 0; i < gd->counts.shadows.offsetCount; i++) {
			auto* el = gd->shadowsOwin.getData() + i;
			gd->shadowsGlobalToLocalMap.emplace(globalToNumericId<GlobalId, NumericId>(el->edgeBeginningId),
			                                    nextShadowLocalId);
			nextShadowLocalId += 1;
		}

		/* extract remapped vertices from window (written by master) to vector returned locally */
		auto remappedVertices = extractRemapedVerticesToVector(gd);
		/* at this point stuff related to remapping could be cleaned up */

		return std::make_pair(new RoundRobin2DPartition<LocalId, NumericId>(gd), remappedVertices);
	};

private:
	std::string path;

	static void destroyGraph(G* g) {
		MPI_Type_free(&(g->graphData->globalIdDt));
		delete g->graphData;
	}
};

#endif //FRAMEWORK_ROUNDROBIN2DPARTITION_H
