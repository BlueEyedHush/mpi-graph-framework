#include <cstdio>
#include <mpi.h>
#include "GraphPartition.h"
#include "representations/SimpleStaticGraph.h"
#include "representations/AdjacencyListHashPartition.h"
#include "algorithms/GraphColouring.h"
#include "algorithms/GraphColouringAsync.h"


/*
 * @ToDo:
 * - get neighbour count
 * - iterator over vertex ids
 * - vertex id string representation (+ use std::string isntead of char arrays)
 * - abstract away vertex id
 * - split vertices between processes
 */

#define WAIT_FOR_DEBUGGER 0

#if WAIT_FOR_DEBUGGER == 1
#include <unistd.h>
#include <signal.h>
#endif

int main() {
	#if WAIT_FOR_DEBUGGER == 1
	fprintf(stderr, "[%d] PID: %d\n", nodeId, getpid());
	raise(SIGSTOP);
	//volatile short execute_loop = 1;
	//while(execute_loop == 1) {}
	#endif

	MPI_Init(NULL, NULL);

	GraphBuilder *graphBuilder = new ALHPGraphBuilder();
	GraphPartition *g = reinterpret_cast<GraphPartition*>(malloc(sizeof(ALHPGraphPartition)));
	g = graphBuilder->buildGraph(std::string("graphs/simple4vertex.adjl"), g);

	Algorithm *algorithm = new GraphColouringMPAsync();
	bool result = algorithm->run(g);

	MPI_Barrier(MPI_COMM_WORLD);

	/* representation & algorithm might use MPI routines in destructor, so need to clean it up before finalizing */
	delete algorithm;
	delete g;
	MPI_Finalize();

	if (!result) {
		fprintf(stderr, "Error occured while executing algorithm\n");
		return 1;
	} else {
		fprintf(stderr, "Algorithm terminated successfully\n");
		return 0;
	}w
}
