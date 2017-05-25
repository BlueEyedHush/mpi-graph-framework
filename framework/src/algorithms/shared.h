//
// Created by blueeyedhush on 25.05.17.
//

#ifndef FRAMEWORK_SHARED_H
#define FRAMEWORK_SHARED_H

#include <cstddef>
#include <set>
#include <mpi.h>
#include <GraphPartition.h>

#define MPI_TAG 0
#define OUTSTANDING_RECEIVE_REQUESTS 5


struct Message {
	LocalVertexId receiving_node_id;
	int used_colour;
};

void register_mpi_message(MPI_Datatype *memory);

struct VertexTempData {
	VertexTempData() : wait_counter(0) {}

	std::set<int> used_colours;
	int wait_counter;
};


#endif //FRAMEWORK_SHARED_H
