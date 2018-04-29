#include <cstdio>
#include <iostream>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <glog/logging.h>
#include <assemblies/ColouringAssembly.h>
#include "Assembly.h"
#include "Executor.h"
#include "representations/ArrayBackedChunkedPartition.h"
#include "representations/AdjacencyListHashPartition.h"
#include "algorithms/colouring/GraphColouringMp.h"
#include "algorithms/colouring/GraphColouringMpAsync.h"
#include "algorithms/bfs/Bfs1CommsRound.h"
#include "validators/ColouringValidator.h"
#include "assemblies/BfsAssembly.h"
#include "validators/BfsValidator.h"

namespace po = boost::program_options;

#define WAIT_FOR_DEBUGGER 0

#if WAIT_FOR_DEBUGGER == 1
#include <unistd.h>
#include <signal.h>
#endif

struct Configuration {
	std::string graphFilePath;
	std::string assemblyName;
};

boost::optional<Configuration> parse_cli_args(const int argc, const char** argv) {
	po::options_description desc("Usage");
	desc.add_options()
			("graph,g", po::value<std::string>(), "graph file to load (mandatory)")
			("assembly,a", po::value<std::string>(), "assembly to execute (mandatory)")
			;

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	Configuration config;

	if (vm.count("graph")) {
		config.graphFilePath = vm["graph"].as<std::string>();
	} else {
		std::cout << desc << std::endl;
		return boost::none;
	}

	if (vm.count("assembly")) {
		config.assemblyName = vm["assembly"].as<std::string>();
	} else {
		std::cout << desc << std::endl;
		return boost::none;
	}

	return config;
}

int main(const int argc, const char** argv) {
	google::InitGoogleLogging(argv[0]);
	FLAGS_logtostderr = true;

	#if WAIT_FOR_DEBUGGER == 1
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	fprintf(stderr, "PID: %d\n", world_rank, getpid());
	raise(SIGSTOP);
	//volatile short execute_loop = 1;
	//while(execute_loop == 1) {}
	#endif

	Configuration config;
	if(auto optConfig = parse_cli_args(argc, argv)) {
		config = optConfig.value();
	} else {
		return 1;
	}

	Executor executor;

	using THandle = ALHGraphHandle<int,int>;
	auto *graphHandle = new THandle(config.graphFilePath, {0L});
	executor.registerAssembly("colouring", new ColouringAssembly<GraphColouringMp, THandle>(*graphHandle));
	executor.registerAssembly("bfs", new BfsAssembly<Bfs_Mp_VarMsgLen_1D_1CommsTag, THandle>(*graphHandle));

	auto an = config.assemblyName;
	if(an.empty() || !executor.executeAssembly(an)) {
		std::cout << "Assembly with name '" << config.assemblyName << "' not found!" << std::endl;
	}

	delete graphHandle;
	return 0;
}
