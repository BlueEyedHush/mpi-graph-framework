#include <cstdio>
#include <cinttypes>
#include <mpi.h>
#include "Graph.h"
#include "representations/SimpleStaticGraph.h"


/*
 * @ToDo:
 * - algorithm abstraction
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
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);


    /* safety checks */

	if (world_size != V_N) {
		fprintf(stderr, "World size (%" SCNd16 ") != vertex nubmer in graph (%" SCNd16 ")\n", world_size, V_N);
		return 1;
	}

    /* gather information about rank of neighbours */

	int wait_counter = 0;
	Iterator<VERTEX_ID_TYPE> *neighIt = g->getNeighbourIterator(world_rank);
	for(VERTEX_ID_TYPE id = 0; neighIt->hasNext(); id = neighIt->next()) {
		if (id > world_rank) {
			wait_counter++;
		}
	}
	delete neighIt;
	fprintf(stderr, "[RANK %" SCNd16 "] Waiting for %" SCNd16 " nodes to establish colouring\n", world_rank, wait_counter);

    /* wait until neighbours establish their colour */

	uint16_t used_colours[V_N] = {0};// @todo lower upper bound can be employed
	while (wait_counter > 0) {
		uint16_t colour = 0;
		MPI_Recv(&colour, 1, MPI_UINT16_T, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		fprintf(stderr, "Received colour %" SCNd16, colour);
		used_colours[colour] = 1;
		wait_counter--;
		fprintf(stderr, "[RANK %" SCNd16 "] Colour %" SCNd16 " used by neighbour. %" SCNd16 " still to go.\n", world_rank, colour, wait_counter);
	}

    /* choose colour */

	uint16_t chosen_colour = 0;
	uint8_t colour_found = 0;
	for(uint16_t i = 0; i < V_N && colour_found == 0; i++) {
		if (used_colours[i] == 0) {
			chosen_colour = i;
			colour_found = 1;
		}
	}
	fprintf(stderr, "[RANK %" SCNd16 "] Colouring with %" SCNd16 "\n", world_rank, chosen_colour);

    /* informing neighbours with lower id */

	neighIt = g->getNeighbourIterator(world_rank);
	for(VERTEX_ID_TYPE id = 0; neighIt->hasNext(); id = neighIt->next()) {
		if(id < world_rank) {
			MPI_Send(&chosen_colour, 1, MPI_UINT16_T, id, 0, MPI_COMM_WORLD);
		}
	}
	delete neighIt;

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();

	delete g;

	return 0;
}
