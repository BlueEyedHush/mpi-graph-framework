#include <stdio.h>
#include <mpi.h>

/* complete graph with 4 vertices */
#define E_N 12
unsigned short E[] = {
		1, 2, 3,
        0, 2, 3,
        0, 1, 3,
        0, 1, 2
};

#define V_N 4
unsigned short V_OFFSETS[] = {
		0,
        3,
        6,
        9
};

struct neighbours {
	unsigned short *start;
	int length;
};

struct neighbours get_neighbours(unsigned short v) {
	struct neighbours n;
	n.start = &E[V_OFFSETS[v]];
	n.length = (v != V_N - 1) ? (V_OFFSETS[v+1] - V_OFFSETS[v]) : (E_N - V_OFFSETS[V_N-1]);
	return n;
}

void graph_test() {
	for(unsigned short i = 0; i < V_N; i++) {
		struct neighbours n = get_neighbours(i);
		for(unsigned short j = 0; j < n.length; j++) {
			unsigned short n_id = n.start[j];
			printf("%d, ", n_id);
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
		fprintf(stderr, "World size (%d) != vertex nubmer in graph (%d)\n", world_size, V_N);
		return 1;
	}

	/* gather information about rank of neighbours */
	int wait_counter = 0;
	struct neighbours n = get_neighbours(world_rank);
	for(unsigned short j = 0; j < n.length; j++) {
		unsigned short n_id = n.start[j];
		if(n_id > world_rank) {
			wait_counter++;
		}
	}
	fprintf(stderr, "[RANK %d] Waiting for %d nodes to establish colouring\n", world_rank, wait_counter);

	/* wait until neighbours establish their colour */
	unsigned short used_colours[V_N] = {0};// @todo lower upper bound can be employed
	while (wait_counter > 0) {
		unsigned short colour = 0;
		MPI_Recv(&colour, 1, MPI_UNSIGNED_SHORT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		used_colours[colour] = 1;
		wait_counter--;
		fprintf(stderr, "[RANK %d] Colour %d used by neighbour. %d still to go.\n", world_rank, colour, wait_counter);
	}

	/* choose colour */
	int chosen_colour = -1;
	for(unsigned short i = 0; i < V_N && chosen_colour == -1; i++) {
		if (used_colours[i] == 0) {
			chosen_colour = i;
		}
	}
	fprintf(stderr, "[RANK %d] Colouring with %d\n", world_rank, chosen_colour);

	/* informing neighbours with lower id */
	for (unsigned short i = 0; i < world_rank; i++) {
		unsigned short n_id = n.start[i];
		unsigned short c = (unsigned short) chosen_colour;
		MPI_Send(&c, 1, MPI_UNSIGNED_SHORT, n_id, 0, MPI_COMM_WORLD);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	MPI_Finalize();
	return 0;
}