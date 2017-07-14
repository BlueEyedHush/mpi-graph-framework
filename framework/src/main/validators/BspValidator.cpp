//
// Created by blueeyedhush on 14.07.17.
//

#include "BspValidator.h"
#include <climits>
#include <unordered_map>
#include <functional>
#include <utils/GrouppingMpiAsync.h>

namespace {
	class DistanceChecker {
		typedef unsigned long long ull;

	public:
		DistanceChecker(GrouppingMpiAsync& _asyncExecutor) : mpiAsync(_asyncExecutor) {}

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
					MPI_Request *rq = new MPI_Request;
					/* @Todo: get buffer */
					int* buffer = nullptr;
					/* @Todo: communication */
					/* create group and schedule housekeepng callback */
					groupId = mpiAsync.createWaitingGroup(rq, [buffer, numId, this]() {
						this->distanceMap[numId] = *buffer;
						this->pending.erase(numId);
						/* @ToDo: clean up buffer */
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
	DistanceChecker dc(executor);

	int checkedCount = 0;
	bool valid = true;
	g->forEachLocalVertex([&dc, &valid, &checkedCount, partialSolution](LocalVertexId id) {
		GlobalVertexId& predecessor = partialSolution[id].first;
		int distanceFromAlgorithm = partialSolution[id].second;
		dc.scheduleIsDistanceAsExpected(predecessor, distanceFromAlgorithm-1, [&valid, &checkedCount](bool distAsExpected) {
			valid = distAsExpected;
			checkedCount += 1;
		});
	});

	while(checkedCount < g->getLocalVertexCount()) {
		executor.poll();
	}

	return valid;
}
