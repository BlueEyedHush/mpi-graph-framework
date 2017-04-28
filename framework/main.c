#include <stdio.h>
#include <inttypes.h>
#include <mpi.h>

/* complete graph with 4 vertices */
#define E_N 12
uint16_t E[] = {
		1, 2, 3,
        0, 2, 3,
        0, 1, 3,
        0, 1, 2
};

#define V_N 4
uint16_t V_OFFSETS[] = {
		0,
        3,
        6,
        9
};

struct neighbours {
	uint16_t *start;
	int length;
};

struct neighbours get_neighbours(uint16_t v) {
	struct neighbours n;
	n.start = &E[V_OFFSETS[v]];
	n.length = (v != V_N - 1) ? (V_OFFSETS[v+1] - V_OFFSETS[v]) : (E_N - V_OFFSETS[V_N-1]);
	return n;
}

void graph_test() {
	for(uint16_t i = 0; i < V_N; i++) {
		struct neighbours n = get_neighbours(i);
		for(uint16_t j = 0; j < n.length; j++) {
			uint16_t n_id = n.start[j];
			printf("%" SCNd16 ", ", n_id);
		}
		printf("\n");
	}
}

/* *** */

int main() {

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
	struct neighbours n = get_neighbours(world_rank);
	for(uint16_t j = 0; j < n.length; j++) {
		uint16_t n_id = n.start[j];
		if(n_id > world_rank) {
			wait_counter++;
		}
	}
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
	for (uint16_t i = 0; i < world_rank; i++) {
		uint16_t n_id = n.start[i];
		MPI_Send(&chosen_colour, 1, MPI_UINT16_T, n_id, 0, MPI_COMM_WORLD);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	return 0;
}