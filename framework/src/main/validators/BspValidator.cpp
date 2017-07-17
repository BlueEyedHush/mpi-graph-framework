//
// Created by blueeyedhush on 14.07.17.
//

#include "BspValidator.h"
#include <climits>
#include <unordered_map>
#include <functional> /* for std::function */
#include <utility> /* for std::pair */
#include <glog/logging.h>
#include <mpi.h>
#include <utils/MPIAsync.h>
#include <utils/GrouppingMpiAsync.h>

namespace {
	/**
	 * Assumes that MPI has already been initialized (and shutdown is handled by caller)
	 */
	class Comms {
	public:
		Comms(GraphPartition *_g, std::pair<GlobalVertexId, int> *partialSolution) : g(_g) {
			MPI_Win_create(partialSolution, g->getMaxLocalVertexCount()*sizeof(int), sizeof(int),
			               MPI_INFO_NULL, MPI_COMM_WORLD, &solutionWin);
			MPI_Win_lock_all(0, solutionWin);
		}

		/**
		 *
		 * @param id
		 * @return when no longer needed use delete (no delete[] !!!) on both buffer and MPI_Request
		 */
		std::pair<int*, MPI_Request*> getDistance(GlobalVertexId id) {
			int* buffer = new int(0);
			MPI_Request *rq = new MPI_Request;
			MPI_Rget(buffer, 1, MPI_INT, id.nodeId, id.localId, 1, MPI_INT, solutionWin, rq);
			return std::make_pair(buffer, rq);
		}

		void flushAll() {
			MPI_Win_flush_all(solutionWin);
		}

		~Comms() {
			MPI_Win_unlock_all(solutionWin);
			MPI_Win_free(&solutionWin);
		}

	private:
		GraphPartition * const g;
		MPI_Win solutionWin;
	};

	class DistanceChecker {
		typedef unsigned long long ull;

	public:
		DistanceChecker(GrouppingMpiAsync& _asyncExecutor, Comms& _comms) : mpiAsync(_asyncExecutor), comms(_comms) {}

		void scheduleIsDistanceAsExpected(GlobalVertexId id, int expectedDistance, std::function<void(bool)> cb) {
			ull numId = toNumerical(id);
			auto it = distanceMap.find(numId);
			if (it != distanceMap.end()) {
				cb(it->second == expectedDistance);
			} else {
				/* if pending, we need only to append; otherwise new waiting group needs to be created */

				/* first, ensure waiting group exists and housekeeping callback is registered */
				auto itPending = pending.find(numId);
				UniqueIdGenerator::Id groupId;
				if(itPending != pending.end()) {
					groupId = itPending->second;
				} else {
					auto p = comms.getDistance(id);
					int* buffer = p.first;
					MPI_Request *rq = p.second;
					/* create group and schedule housekeepng callback */
					groupId = mpiAsync.createWaitingGroup(rq, [buffer, numId, this]() {
						this->distanceMap[numId] = *buffer;
						this->pending.erase(numId);
						delete buffer;
					});
					this->pending.insert(std::make_pair(numId, groupId));
				}

				/* add actual verification callback */
				auto verificationCb = [expectedDistance, numId, cb, this](){
					int actualDist = this->distanceMap.at(numId);
					cb(actualDist == expectedDistance);
				};
				mpiAsync.addToGroup(groupId, verificationCb);
			}
		}

	private:
		std::unordered_map<ull, int> distanceMap;
		std::unordered_map<ull, UniqueIdGenerator::Id> pending;
		GrouppingMpiAsync& mpiAsync;
		Comms &comms;
	private:
		ull toNumerical(GlobalVertexId id) {
			unsigned int halfBitsInUll = (sizeof(ull)*CHAR_BIT)/2;
			ull numerical = ((ull) id.localId) << halfBitsInUll;
			numerical |= ((unsigned int) id.nodeId);
			return numerical;
		}

	};
}

bool BspValidator::validate(GraphPartition *g, std::pair<GlobalVertexId, int> *partialSolution) {
	GrouppingMpiAsync executor;
	Comms comms(g, partialSolution);
	DistanceChecker dc(executor, comms);

	int checkedCount = 0;
	bool valid = true;
	g->forEachLocalVertex([&dc, &valid, &checkedCount, partialSolution, g](LocalVertexId id) {
		GlobalVertexId v(g->getNodeId(), id);
		GlobalVertexId& predecessor = partialSolution[id].first;
		int distanceFromAlgorithm = partialSolution[id].second;
		dc.scheduleIsDistanceAsExpected(predecessor, distanceFromAlgorithm-1, [&valid, &checkedCount, v, &predecessor](bool distAsExpected) {
			valid = distAsExpected;
			checkedCount += 1;

			if(!distAsExpected) {
				LOG(INFO) << "Failure for " << v.toString() << "."
				          << "Distance to predecessor " << predecessor.toString() << " was " << 1 << " instead of 1";
			}
		});
	});

	comms.flushAll();
	while(checkedCount < g->getLocalVertexCount()) {
		executor.poll();
	}

	return valid;
}
