//
// Created by blueeyedhush on 06.05.17.
//

#include <cstdio>
#include <cstring>
#include <mpi.h>
#include "GraphColouring.h"

bool GraphColouring::run(Graph *g) {
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);


	/* safety checks */

	if (world_size != g->getVertexCount()) {
		fprintf(stderr, "World size (%" SCNd16 ") != vertex nubmer in graph (%" SCNd16 ")\n", world_size, g->getVertexCount());
		return false;
	}

	/* gather information about rank of neighbours */

	int wait_counter = 0;
	VERTEX_ID_TYPE id = 0;
	Iterator<VERTEX_ID_TYPE> *neighIt = g->getNeighbourIterator(world_rank);
	while(neighIt->hasNext()) {
		id = neighIt->next();
		if (id > world_rank) {
			wait_counter++;
		}
	}
	fprintf(stderr, "[RANK %" SCNd16 "] Waiting for %" SCNd16 " nodes to establish colouring\n", world_rank, wait_counter);

	/* wait until neighbours establish their colour */

	SIZE_TYPE *used_colours = new SIZE_TYPE[g->getVertexCount()];// @todo lower upper bound can be employed
	memset(used_colours, 0, sizeof(SIZE_TYPE)*g->getVertexCount());
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
	for(uint16_t i = 0; i < g->getVertexCount() && colour_found == 0; i++) {
		if (used_colours[i] == 0) {
			chosen_colour = i;
			colour_found = 1;
		}
	}
	fprintf(stderr, "[RANK %" SCNd16 "] Colouring with %" SCNd16 "\n", world_rank, chosen_colour);
	delete[] used_colours;

	/* informing neighbours with lower id */

	id = 0;
	neighIt = g->getNeighbourIterator(world_rank);
	while(neighIt->hasNext()) {
		id = neighIt->next();
		if(id < world_rank) {
			MPI_Send(&chosen_colour, 1, MPI_UINT16_T, id, 0, MPI_COMM_WORLD);
		}
	}

	return true;
}
