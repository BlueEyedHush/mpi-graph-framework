#include <cstdio>
#include <iostream>
#include <glog/logging.h>
#include <utils/Config.h>
#include "Assembly.h"
#include "Executor.h"
#include "representations/ArrayBackedChunkedPartition.h"
#include "representations/AdjacencyListHashPartition.h"
#include "algorithms/colouring/GraphColouringMp.h"
#include "algorithms/colouring/GraphColouringMpAsync.h"
#include "algorithms/bfs/Bfs1CommsRound.h"
#include "validators/ColouringValidator.h"
#include <assemblies/ColouringAssembly.h>
#include "assemblies/BfsAssembly.h"
#include <assemblies/RepeatingAssembly.h>
#include "validators/BfsValidator.h"

#define WAIT_FOR_DEBUGGER 0

#if WAIT_FOR_DEBUGGER == 1
#include <unistd.h>
#include <signal.h>
#endif

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

	ConfigMap cm = parseCli(argc, argv);
	auto assemblyName = cm["a"];
	auto graphFilePath = cm["g"];
	
	Executor executor;

	using THandle = ALHGraphHandle<int,int>;
	auto *graphHandle = new THandle(graphFilePath, {0L});
	executor.registerAssembly("colouring", new ColouringAssembly<GraphColouringMp, THandle>(*graphHandle));
	executor.registerAssembly("bfs", new BfsAssembly<Bfs_Mp_VarMsgLen_1D_1CommsTag, THandle>(*graphHandle));
	executor.registerAssembly("repeating", new RepeatingAssembly());

	if(assemblyName.empty() || !executor.executeAssembly(assemblyName)) {
		std::cout << "Assembly with name '" << assemblyName << "' not found!" << std::endl;
	}

	delete graphHandle;
	return 0;
}
