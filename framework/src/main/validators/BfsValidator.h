//
// Created by blueeyedhush on 14.07.17.
//

#ifndef FRAMEWORK_BFSVALIDATOR_H
#define FRAMEWORK_BFSVALIDATOR_H

#include <utility>
#include <climits>
#include <unordered_map>
#include <functional> /* for std::function */
#include <utility> /* for std::pair */
#include <glog/logging.h>
#include <mpi.h>
#include <Validator.h>
#include <utils/MPIAsync.h>
#include <utils/GrouppingMpiAsync.h>
#include <utils/MpiTypemap.h>

namespace details {
	/**
	 * Assumes that MPI has already been initialized (and shutdown is handled by caller)
	 */
	template <typename TGraphPartition, typename TGlobalId>
	class Comms {
	public:
		Comms(TGraphPartition *_g, std::pair<TGlobalId*, GraphDist*> partialSolution) : g(_g) {
			MPI_Win_create(partialSolution.second, g->masterVerticesMaxCount()*sizeof(GraphDist), sizeof(GraphDist),
			               MPI_INFO_NULL, MPI_COMM_WORLD, &solutionWin);
			MPI_Win_lock_all(0, solutionWin);
		}

		/**
		 *
		 * @param id
		 * @return when no longer needed use delete (no delete[] !!!) on both buffer and MPI_Request
		 */
		std::pair<GraphDist*, MPI_Request*> getDistance(const TGlobalId id) const {
			GraphDist* buffer = new GraphDist(0);
			MPI_Request *rq = new MPI_Request;
			auto gdMpiType = getDatatypeFor<GraphDist>();
			MPI_Rget(buffer, 1, gdMpiType, g->toMasterNodeId(id), g->toLocalId(id), 1, gdMpiType, solutionWin, rq);
			return std::make_pair(buffer, rq);
		}

		bool checkAllResults(bool &currentNodeValid) const {
			bool collectiveResult = false;
			MPI_Allreduce(&currentNodeValid, &collectiveResult, 1, MPI_CXX_BOOL, MPI_LAND, MPI_COMM_WORLD);
			return collectiveResult;
		}

		void flushAll() {
			MPI_Win_flush_all(solutionWin);
		}

		~Comms() {
			MPI_Win_unlock_all(solutionWin);
			MPI_Win_free(&solutionWin);
		}

	private:
		TGraphPartition * const g;
		MPI_Win solutionWin;
	};

	template <typename TGraphPartition, typename TGlobalId>
	class DistanceChecker {
		typedef unsigned long long ull;

	public:
		DistanceChecker(GrouppingMpiAsync& _asyncExecutor,
		                Comms<TGraphPartition, TGlobalId>& _comms,
		                TGraphPartition& g)
				: mpiAsync(_asyncExecutor), comms(_comms), g(g) {}

		void scheduleGetDistance(const TGlobalId id, std::function<void(GraphDist)> cb) {
			ull numId = g.toNumeric(id);
			auto it = distanceMap.find(numId);
			if (it != distanceMap.end()) {
				cb(it->second);
			} else {
				/* if pending, we need only to append; otherwise new waiting group needs to be created */

				/* first, ensure waiting group exists and housekeeping callback is registered */
				auto itPending = pending.find(numId);
				UniqueIdGenerator::Id groupId;
				if(itPending != pending.end()) {
					groupId = itPending->second;
				} else {
					auto p = comms.getDistance(id);
					GraphDist* buffer = p.first;
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
				auto verificationCb = [numId, cb, this](){
					GraphDist actualDist = this->distanceMap.at(numId);
					cb(actualDist);
				};
				mpiAsync.addToGroup(groupId, verificationCb);
			}
		}

	private:
		std::unordered_map<ull, GraphDist> distanceMap;
		std::unordered_map<ull, UniqueIdGenerator::Id> pending;
		GrouppingMpiAsync& mpiAsync;
		Comms<TGraphPartition, TGlobalId> &comms;
		TGraphPartition& g;
	};
}

template <class TGraphPartition>
class BfsValidator : public Validator<TGraphPartition, std::pair<typename TGraphPartition::GidType*, GraphDist*>*> {
private:
	IMPORT_ALIASES(TGraphPartition)

public:
	BfsValidator(const GlobalId _root) : root(_root) {};

	// @ToDo - (types) path length should be parametrizable + registering type with MPI
	bool validate(TGraphPartition *g, std::pair<GlobalId*, GraphDist*> *partialSolution) {
		GrouppingMpiAsync executor;
		details::Comms<TGraphPartition, GlobalId> comms(g, *partialSolution);
		details::DistanceChecker<TGraphPartition, GlobalId> dc(executor, comms, *g);

		int checkedCount = 0;
		bool valid = true;
		g->foreachMasterVertex([&dc, &valid, &checkedCount, partialSolution, g, this](const LocalId id) {
			const GlobalId currGID = g->toGlobalId(id);
			const GlobalId predecessor = partialSolution->first[id];
			GraphDist actualDistance = partialSolution->second[id];

			/* check if distance positive */
			if(actualDistance < 0) {
				LOG(INFO) << "Failure for " << g->idToString(currGID) << "(precedessor: " << g->idToString(predecessor)
				          << "): distance (" << actualDistance << ") is negative";
				valid = false;
			}

			/* check if difference in predecessor and successor distance equals 1 (or if correct node is root) */
			if(!g->isSame(predecessor, currGID)) {
				auto checkDistCb =
				[&valid, &checkedCount, g, &currGID, &predecessor, actualDistance](GraphDist predecessorDistance) {
					GraphDist expectedPrecedessorDist = actualDistance-1;
					bool thisValid = expectedPrecedessorDist == predecessorDistance;

					if(!thisValid) {
						LOG(INFO) << "Failure for " << g->idToString(currGID) << ". "
						          << "Precedessor: " << g->idToString(predecessor)
						          << ", ourDistance: " << actualDistance
						          << ", predecessorDistance: " << predecessorDistance
						          << ", expectedPredecessorDistance: " << expectedPrecedessorDist;
					}

					valid = valid && thisValid;
					checkedCount += 1;
				};

				dc.scheduleGetDistance(predecessor, checkDistCb);
			} else {
				/* only root node can have himself as a predecessor */
				if(!g->isSame(currGID, root)) {
					LOG(INFO) << g->idToString(predecessor) << " reported as root (correct root: "
					          << g->idToString(root) << " )" << std::endl;

					valid = false;
				}

				checkedCount += 1;
			}

			return ITER_PROGRESS::CONTINUE;
		});

		comms.flushAll();
		while(checkedCount < g->masterVerticesCount()) {
			executor.poll();
		}

		return comms.checkAllResults(valid);
	}

private:
	const GlobalId root;
};


#endif //FRAMEWORK_BFSVALIDATOR_H
