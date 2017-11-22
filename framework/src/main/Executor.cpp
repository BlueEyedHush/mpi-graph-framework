//
// Created by blueeyedhush on 22.11.17.
//

#include <boost/program_options/variables_map.hpp>
#include "Executor.h"

Executor::Executor(std::function<void(Assembly*)> assemblyCleaner) {
	MPI_Init(NULL, NULL);

	int currentNodeId;
	MPI_Comm_rank(MPI_COMM_WORLD, &currentNodeId);
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	LOG(INFO) << "NODE_ID: " << currentNodeId << " WORLD_SIZE: " << worldSize;
}

Executor::~Executor() {
	MPI_Finalize();
}

void Executor::registerAssembly(const std::string key, Assembly* assembly) {
	assemblies.emplace(key, assembly);
}

bool Executor::executeAssembly(const std::string key) {
	if (assemblies.find(key) != assemblies.end()) {
		LOG(INFO) << "Executing assembly: " << key;
		auto* assembly = assemblies.at(key);
		assembly->run();
		return true;
	} else {
		return false;
	}
}

