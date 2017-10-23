//
// Created by blueeyedhush on 23.10.17.
//

#ifndef FRAMEWORK_COLOURING_H
#define FRAMEWORK_COLOURING_H

#include <mpi.h>
#include <Algorithm.h>

typedef int VertexColour;
const MPI_Datatype VERTEX_COLOUR_MPI_TYPE = MPI_INT;

namespace details {
	const int MPI_TAG = 0;
	const size_t OUTSTANDING_RECEIVE_REQUESTS = 5;

	struct Message {
		LocalVertexId receiving_node_id;
		int used_colour;

		static MPI_Datatype mpiDatatype() {
			MPI_Datatype d;
			int blocklengths[] = {1, 1};
			MPI_Aint displacements[] = {offsetof(Message, receiving_node_id), offsetof(Message, used_colour)};
			MPI_Datatype building_types[] = {MPI_INT, MPI_INT};
			MPI_Type_create_struct(2, blocklengths, displacements, building_types, &d);

			return d;
		}
	};

	struct VertexTempData {
		VertexTempData() : wait_counter(0) {}

		std::set<int> used_colours;
		int wait_counter;
	};
}

#endif //FRAMEWORK_COLOURING_H
