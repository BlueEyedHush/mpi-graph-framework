//
// Created by blueeyedhush on 23.10.17.
//

#ifndef FRAMEWORK_COLOURING_H
#define FRAMEWORK_COLOURING_H

#include <cstddef>
#include <mpi.h>
#include <Algorithm.h>

typedef int VertexColour;
const MPI_Datatype VERTEX_COLOUR_MPI_TYPE = MPI_INT;

namespace details {
	const int MPI_TAG = 0;
	const size_t OUTSTANDING_RECEIVE_REQUESTS = 5;

	template <typename LocalId>
	struct Message {
		LocalId receiving_node_id;
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

		std::set<VertexColour> used_colours;
		int wait_counter;
	};
}

template <class TGraphPartition>
class GraphColouring : public Algorithm<VertexColour*, TGraphPartition> {
public:
	GraphColouring() : finalColouring(nullptr) {}

	virtual bool run(TGraphPartition *g, AAuxiliaryParams aParams) = 0;
	/**
	 *  This method should only return part of the result that is local to the node
	 *  If the result is represented by array with 1:1 vertex-result mapping, it should allocate
	 *   g->getMaxLocalVertexCount(), even if number of vertices assigned to current partition is smaller
	 *
	 *  Returned result should be cleaned on object destruction
	 *
	 *  Don't use types for which you cannot create matching MPI Derived Type - it'll make creation of validator harder
	 *
	 */
	virtual VertexColour* getResult() {
		return finalColouring;
	};

	virtual ~GraphColouring() {
		if(finalColouring != nullptr) delete[] finalColouring;
	};

protected:
	VertexColour* finalColouring;
};

#endif //FRAMEWORK_COLOURING_H
