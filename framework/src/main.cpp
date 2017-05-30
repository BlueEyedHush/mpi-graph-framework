#include <cstdio>
#include <mpi.h>
#include "GraphPartition.h"
#include "representations/SimpleStaticGraph.h"
#include "representations/AdjacencyListHashPartition.h"
#include "algorithms/GraphColouring.h"
#include "algorithms/GraphColouringAsync.h"
#include "Validator.h"
#include "validators/ColouringValidator.h"


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

	Algorithm<int*> *algorithm = new GraphColouringMPAsync();
	bool result = algorithm->run(g);

	MPI_Barrier(MPI_COMM_WORLD);

	if (!result) {
		fprintf(stderr, "Error occured while executing algorithm\n");
	} else {
		fprintf(stderr, "Algorithm terminated successfully\n");
	}

	Validator<int*> *validator = new ColouringValidator();
	bool validationSuccessfull = validator->validate(g, algorithm->getResult());
	if(!validationSuccessfull) {
		fprintf(stderr, "Validation failure\n");
	} else {
		fprintf(stderr, "Validation success\n");
	}

	/* representation & algorithm might use MPI routines in destructor, so need to clean it up before finalizing */
	delete algorithm;
	delete validator;
	delete g;
	MPI_Finalize();

	return 0;
}
