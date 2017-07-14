//
// Created by blueeyedhush on 14.07.17.
//

#include "UniqueIdGenerator.h"
#include <climits>
#include <glog/logging.h>

UniqueIdGenerator::Id UniqueIdGenerator::next() {
	if (nextId == UINT_MAX) {
		/* logging to FATAL automatically terminates program */
		LOG(FATAL) << "Next query for unique id will exhaust available pool!";
	}

	return nextId++;
}
