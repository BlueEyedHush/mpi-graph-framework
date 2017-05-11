//
// Created by blueeyedhush on 06.05.17.
//

#include <cstdio>
#include <cstring>
#include <mpi.h>
#include "GraphColouring.h"

#define COLOUR_TYPE uint16_t
#define COLOUR_MPI_TYPE MPI_UINT16_T
#define SIZE_MPI_TYPE MPI_UINT16_T

/*
 * @ToDo
 * - less storage for used colour array
 * - async operations
 */

bool GraphColouring::run(Graph *g) {
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	MPI_Group world_group;
	MPI_Comm_group(MPI_COMM_WORLD, &world_group);

	MPI_Win neighbours_processed_w;
	SIZE_TYPE neighbours_processed = 0;
	MPI_Win_create(&neighbours_processed, sizeof(SIZE_TYPE), sizeof(SIZE_TYPE), MPI_INFO_NULL, MPI_COMM_WORLD,
	               &neighbours_processed_w);
	MPI_Win_fence(0, neighbours_processed_w);

	COLOUR_TYPE *used_colours = new COLOUR_TYPE[g->getVertexCount()];
	MPI_Win used_colours_w;
	MPI_Win_create(used_colours, sizeof(COLOUR_TYPE)*g->getVertexCount(), sizeof(COLOUR_TYPE), MPI_INFO_NULL, MPI_COMM_WORLD, &used_colours_w);

	memset(used_colours, 0, sizeof(SIZE_TYPE)*g->getVertexCount());
	MPI_Win_post(world_group, 0, used_colours_w);

	/* gather information about rank of neighbours */

	SIZE_TYPE higher_id_neighbour_count = 0;
	Iterator<VERTEX_ID_TYPE> *neighIt = g->getNeighbourIterator(world_rank);
	while(neighIt->hasNext()) {
		VERTEX_ID_TYPE id = neighIt->next();
		if (id > world_rank) {
			higher_id_neighbour_count++;
		}
	}
	delete neighIt;
	fprintf(stderr, "[RANK %" SCNd16 "] Waiting for %" SCNd16 " nodes to establish colouring\n", world_rank, higher_id_neighbour_count);

	/* wait until neighbours establish their colour */

	bool all_neighbours_chose_colour = (higher_id_neighbour_count == 0);
	SIZE_TYPE local_neighbours_processed = 0;
	int dummy = -1;
	while (!all_neighbours_chose_colour) {
		MPI_Get_accumulate(&dummy, 1, MPI_INT, // no-op, so this is not important
		                   &local_neighbours_processed, 1, SIZE_MPI_TYPE,
		                   world_rank, 0, 1, SIZE_MPI_TYPE,
		                   MPI_NO_OP, neighbours_processed_w);

		//fprintf(stderr, "[RANK %" SCNd16 "] Accumulate result: %" SCNd8 "\n", world_rank, local_neighbours_processed);

		if (local_neighbours_processed == higher_id_neighbour_count) {
			all_neighbours_chose_colour = true;
		}

		/* sleep here somehow */
	}

	fprintf(stderr, "[RANK %" SCNd16 "] All higher id neighbours coloured.\n", world_rank);

	/* choose colour */

	MPI_Win_wait(used_colours_w); // why hangs here???
	COLOUR_TYPE chosen_colour = 0;
	bool colour_found = 0;
	for(SIZE_TYPE i = 0; i < g->getVertexCount() && !colour_found; i++) {
		if (used_colours[i] == 0) {
			chosen_colour = i;
			colour_found = true;
		}
	}
	fprintf(stderr, "[RANK %" SCNd16 "] Colouring with %" SCNd16 "\n", world_rank, chosen_colour);

	/* informing neighbours with lower id */

	neighIt = g->getNeighbourIterator(world_rank);
	while(neighIt->hasNext()) {
		VERTEX_ID_TYPE id = neighIt->next();
		if(id < world_rank) {
			COLOUR_TYPE used_marker = 1;
			/* mark your colour as used */
			MPI_Win_start(world_group, 0, used_colours_w);
			MPI_Put(&used_marker, 1, COLOUR_MPI_TYPE, id, chosen_colour, 1, COLOUR_MPI_TYPE, used_colours_w);
			MPI_Win_complete(used_colours_w);

			/* increment his counter of processed neighbours */
			SIZE_TYPE one = 1;
			MPI_Accumulate(&one, 1, SIZE_MPI_TYPE, id, 0, 1, SIZE_MPI_TYPE, MPI_SUM, neighbours_processed_w);
			fprintf(stderr, "[RANK %" SCNd16 "] Informing %" SCNd16 " about changes\n", world_rank, id);
		}
	}
	delete neighIt;

	/*
	 * Fatal error in MPI_Win_free: Wrong synchronization of RMA calls , error stack:
	 * MPI_Win_free(134).: MPI_Win_free(win=0x7ffce8d511ac) failed
	 * MPIDI_Win_free(49): Wrong synchronization of RMA calls
	 */
	MPI_Win_free(&neighbours_processed_w);
	MPI_Win_free(&used_colours_w);
	delete[] used_colours;

	fprintf(stderr, "[RANK %" SCNd16 "] Terminating\n", world_rank);
}
