#include <cstdio>
#include <mpi.h>
#include "Graph.h"
#include "representations/SimpleStaticGraph.h"
#include "algorithms/GraphColouring.h"


/*
 * @ToDo:
 * - get neighbour count
 * - iterator over vertex ids
 * - vertex id string representation (+ use std::string isntead of char arrays)
 * - abstract away vertex id
 * - split vertices between processes
 */

/*int main() {
	Graph *g = new SimpleStaticGraph();
	printGraph(g);
	delete g;
}*/

int main() {
	Graph *g = new SimpleStaticGraph();

	MPI_Init(NULL, NULL);

	Algorithm *algorithm = new GraphColouring();
	bool result = algorithm->run(g);

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	delete g;

	if (!result) {
		fprintf(stderr, "Error occured while executing algorithm\n");
		return 1;
	} else {
		fprintf(stderr, "Algorithm terminated successfully\n");
		return 0;
	}
}
