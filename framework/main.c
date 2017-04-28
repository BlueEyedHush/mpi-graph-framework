#include <stdio.h>
#include <mpi.h>

/* complete graph with 4 vertices */
size_t E_N = 12;
unsigned short E[] = {
		1, 2, 3,
        0, 2, 3,
        0, 1, 3,
        0, 1, 2
};

size_t V_N = 4;
unsigned short V_OFFSETS[] = {
		0,
        3,
        6,
        9
};

struct neighbours {
	unsigned short *start;
	size_t length;
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

int main() {

	MPI_Init(NULL, NULL);
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	int world_size;
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);

	printf("Hello world. Rank: %d, all: %d\n", world_rank, world_size);
	graph_test();

	MPI_Finalize();
	return 0;
}