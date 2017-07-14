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
					groupId = mpiAsync.createWaitingGroup(rq, [buffer, numId, &distanceMap, &pending]() {
						distanceMap[numId] = *buffer;
						pending.erase(numId);
						/* @ToDo: clean up buffer */
					});
					pending.insert(std::make_pair(numId, groupId));
				}

				/* add actual verification callback */
				auto verificationCb = [expectedDistance, numId, cb](){
					int actualDist = distanceMap.at(numId);
					cb(actualDist == expectedDistance);
				};
				mpiAsync.addToGroup(groupId, verificationCb);
			}
		}

	private:
		std::unordered_map<ull, int> distanceMap;
		std::unordered_map<ull, UniqueIdGenerator::Id> pending;
		GrouppingMpiAsync mpiAsync;
	private:
		ull toNumerical(GlobalVertexId id) {
			unsigned int halfBitsInUll = (sizeof(ull)*CHAR_BIT)/2;
			ull numerical = ((ull) id.localId) << halfBitsInUll;
			numerical |= ((unsigned int) id.nodeId);
			return numerical;
		}

	};
}

bool BspValidator::validate(GraphPartition *g, GlobalVertexId *partialSolution) {
	return false;
}
