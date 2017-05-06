#include <cstdio>
#include <cinttypes>
#include <mpi.h>

#define VERTEX_ID_TYPE uint16_t
#define SIZE_TYPE uint16_t

template <class T> class Iterator {
public:
	virtual bool hasNext() = 0;
	virtual T next() = 0;
	virtual ~Iterator() {};
};

/*
 * @ToDo:
 * - split into multiple files
 * - get neighbour count
 * - iterator over vertex ids
 * - vertex id string representation (+ use std::string isntead of char arrays)
 * - algorithm abstraction
 * - abstract away vertex id
 * - split vertices between processes
 */

class Graph {
public:
	virtual SIZE_TYPE getVertexCount() = 0;
	virtual SIZE_TYPE getEdgeCount() = 0;

	virtual Iterator<VERTEX_ID_TYPE> *getNeighbourIterator(VERTEX_ID_TYPE vertexId) = 0;

	virtual ~Graph() {};
};

void printGraph(Graph *g) {
	for(SIZE_TYPE i = 0; i < g->getVertexCount(); i++) {
		printf("%" SCNd16 ": ", i);
		Iterator<VERTEX_ID_TYPE> *neighIt = g->getNeighbourIterator(i);
		for(VERTEX_ID_TYPE id = 0; neighIt->hasNext(); id = neighIt->next()) {
			printf("%" SCNd16 ", ", id);
		}
		delete neighIt;
		printf("\n");
	}
}

/* *** */

/* complete graph with 4 vertices */
#define E_N 12
#define V_N 4

class SimpleStaticGraph : public Graph {
private:
	static constexpr VERTEX_ID_TYPE E[E_N] = {
			1, 2, 3,
			0, 2, 3,
			0, 1, 3,
			0, 1, 2
	};

	static constexpr VERTEX_ID_TYPE V_OFFSETS[V_N] = {
			0,
			3,
			6,
			9
	};

	class NeighIt : public Iterator<VERTEX_ID_TYPE> {
	private:
		VERTEX_ID_TYPE nextId;
		const VERTEX_ID_TYPE *neighbours;
		SIZE_TYPE count;

	public:
		NeighIt(VERTEX_ID_TYPE v) : nextId(0) {
			neighbours = &E[V_OFFSETS[v]];
			count = nextId < (v != V_N - 1) ? (V_OFFSETS[v+1] - V_OFFSETS[v]) : (E_N - V_OFFSETS[V_N-1]);
		}

		virtual VERTEX_ID_TYPE next() override {
			VERTEX_ID_TYPE el = neighbours[nextId];
			nextId++;
			return el;
		}

		virtual bool hasNext() override {
			return nextId < count;
		}

		virtual ~NeighIt() override {}
	};

public:
	virtual SIZE_TYPE getVertexCount() override {
		return V_N;
	}

	virtual SIZE_TYPE getEdgeCount() override {
		return E_N;
	}

	/*
	 * User is responsible for removing iterator
	 */
	virtual Iterator<VERTEX_ID_TYPE> *getNeighbourIterator(VERTEX_ID_TYPE vertexId) {
		return new NeighIt(vertexId);
	}

};

constexpr VERTEX_ID_TYPE SimpleStaticGraph::V_OFFSETS[V_N];
constexpr VERTEX_ID_TYPE SimpleStaticGraph::E[E_N];

/* *** */


/* *** */

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
