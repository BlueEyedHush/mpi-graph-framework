#include <cstdio>
#include <iostream>
#include <mpi.h>
#include <boost/program_options.hpp>
#include <boost/optional.hpp>
#include <glog/logging.h>
#include "GraphPartition.h"
#include "representations/ArrayBackedChunkedPartition.h"
#include "representations/AdjacencyListHashPartition.h"
#include "algorithms/GraphColouring.h"
#include "algorithms/GraphColouringAsync.h"
#include "algorithms/Bsp.h"
#include "Validator.h"
#include "validators/ColouringValidator.h"
#include "validators/BspValidator.h"

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

	GraphBuilder *graphBuilder = new ALHPGraphBuilder();
	GraphPartition *g = reinterpret_cast<GraphPartition*>(malloc(sizeof(ALHPGraphPartition)));
	g = graphBuilder->buildGraph(config.graphFilePath, g);

	auto *algorithm = new Bsp_Mp_FixedMessageSize_1D_2CommRounds();
	bool result = algorithm->run(g);

	MPI_Barrier(MPI_COMM_WORLD);

	if (!result) {
		LOG(ERROR) << "Error occured while executing algorithm";
	} else {
		LOG(INFO) << "Algorithm terminated successfully";
	}

	auto validator = new BspValidator();
	bool validationSuccessfull = validator->validate(g, algorithm->getResult());
	if(!validationSuccessfull) {
		LOG(ERROR) << "Validation failure";
	} else {
		LOG(INFO) << "Validation success";
	}

	/* representation & algorithm might use MPI routines in destructor, so need to clean it up before finalizing */
	delete algorithm;
	delete validator;
	delete g;
	MPI_Finalize();

	return 0;
}
