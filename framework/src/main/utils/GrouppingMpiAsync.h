//
// Created by blueeyedhush on 14.07.17.
//

#ifndef FRAMEWORK_GROUPPINGMPIASYNC_H
#define FRAMEWORK_GROUPPINGMPIASYNC_H

#include <functional>
#include <unordered_map>
#include <vector>
#include <mpi.h>
#include "UniqueIdGenerator.h"

class GrouppingMpiAsync {
public:
	GrouppingMpiAsync(std::function<void(MPI_Request*)> cleaner = defaultCleaner);
	~GrouppingMpiAsync();

	UniqueIdGenerator::Id createWaitingGroup(MPI_Request* rq);
	UniqueIdGenerator::Id createWaitingGroup(MPI_Request* rq, std::function<void(void)> callback);
	void addToGroup(UniqueIdGenerator::Id groupId, std::function<void(void)> callback);
	void poll();

private:
	struct El {
		El() : rq(nullptr) {}
		El(MPI_Request *_rq) : rq(_rq) {}
		El(const El& o) : rq(o.rq), callbacks(o.callbacks) {}
		El& operator=(const El& o) {
			rq = o.rq;
			callbacks = o.callbacks;
			return *this;
		}

		MPI_Request *rq;
		std::vector<std::function<void(void)>> callbacks;
	};

	static std::function<void(MPI_Request*)> defaultCleaner;
	std::function<void(MPI_Request*)> cleaner;

	UniqueIdGenerator idGenerator;
	std::unordered_map<UniqueIdGenerator::Id, El> groupsMap;
};


#endif //FRAMEWORK_GROUPPINGMPIASYNC_H
