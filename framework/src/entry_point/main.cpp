#include <cstdio>
#include <iostream>
#include <mpi.h>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <glog/logging.h>
#include "Assembly.h"
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
};

boost::optional<Configuration> parse_cli_args(const int argc, const char** argv) {
	po::options_description desc("Usage");
	desc.add_options()
			("graph,g", po::value<std::string>(), "name of graph file to load")
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

	return config;
}

int main(const int argc, const char** argv) {
	google::InitGoogleLogging(argv[0]);
	FLAGS_logtostderr = true;

	MPI_Init(NULL, NULL);

	#if WAIT_FOR_DEBUGGER == 1
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	fprintf(stderr, "[%d] PID: %d\n", world_rank, getpid());
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

	int currentNodeId;
	MPI_Comm_rank(MPI_COMM_WORLD, &currentNodeId);
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD, &worldSize);
	LOG(INFO) << "NODE_ID: " << currentNodeId << " WORLD_SIZE: " << worldSize;

	using THandle = ALHGraphHandle<int,int>;
	using TGraph = THandle::GPType;
	using TAlgo = Bfs_Mp_VarMsgLen_1D_1CommsTag<TGraph>;
	using TValid = BfsValidator<TGraph>;

	auto *graphHandle = new THandle(config.graphFilePath, {0L});

	/* to force lifetime of assembly, which must be destroyed before MPI_Finalize */
	{
		BfsAssembly<Bfs_Mp_VarMsgLen_1D_1CommsTag, THandle> assembly(*graphHandle);
		assembly.run();
	}

	/* representation & algorithm might use MPI routines in destructor, so need to clean it up before finalizing */
	graphHandle->releaseGraph();
	delete graphHandle;
	MPI_Finalize();

	return 0;
}
