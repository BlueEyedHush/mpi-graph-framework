//
// Created by blueeyedhush on 14.07.17.
//

#include "GrouppingMpiAsync.h"
#include <glog/logging.h>

std::function<void(MPI_Request*)> GrouppingMpiAsync::defaultCleaner = [](MPI_Request* rq) {
	delete rq;
};

GrouppingMpiAsync::GrouppingMpiAsync(std::function<void(MPI_Request *)> _cleaner) : cleaner(_cleaner) {

}

GrouppingMpiAsync::~GrouppingMpiAsync() {
	if(groupsMap.size() > 0) {
		LOG(WARNING) << "Pending requests left in GrouppingMpiAsync, but destructor has been called. Canceling them";

		for(auto& kv: groupsMap) {
			El& el = kv.second;
			MPI_Cancel(el.rq);
			cleaner(el.rq);
		}
	}
}

UniqueIdGenerator::Id GrouppingMpiAsync::createWaitingGroup(MPI_Request *rq) {
	UniqueIdGenerator::Id id = idGenerator.next();
	El el(rq);
	groupsMap.insert(std::make_pair(id, el));

	return id;
}

UniqueIdGenerator::Id GrouppingMpiAsync::createWaitingGroup(MPI_Request *rq, std::function<void(void)> callback) {
	UniqueIdGenerator::Id id = idGenerator.next();
	El el(rq);
	el.callbacks.push_back(callback);
	groupsMap.insert(std::make_pair(id, el));

	return id;
}

void GrouppingMpiAsync::addToGroup(UniqueIdGenerator::Id groupId, std::function<void(void)> callback) {
	auto elIt = groupsMap.find(groupId);
	if (elIt != groupsMap.end()) {
		elIt->second.callbacks.push_back(callback);
	} else {
		LOG(WARNING) << "Trying to addToGroup which doesn't exist";
	}
}

void GrouppingMpiAsync::poll() {
	auto it = groupsMap.begin();
	/*
	 * requires C++14
	 * https://stackoverflow.com/q/38468844/3182262
	 * */
	while(it != groupsMap.end()) {
		El& el = it->second;

		int result = 0;
		MPI_Test(el.rq, &result, MPI_STATUS_IGNORE);
		if (result != 0) {
			MPI_Wait(el.rq, MPI_STATUS_IGNORE);

			for(auto &cb: el.callbacks) {
				cb();
			}

			it = groupsMap.erase(it);
		} else {
			++it;
		}

	}
}

