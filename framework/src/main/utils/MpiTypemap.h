//
// Created by blueeyedhush on 17.10.17.
//

#ifndef MPI_TYPEMAP_H
#define MPI_TYPEMAP_H

#include <mpi.h>
#include <typeinfo>
#include <typeindex>
#include <unordered_map>

/* to simplify creation of templates - without this map MPI_Datatype would have to be passed as
 * additional non-type argument
 */
extern std::unordered_map<std::type_index, int> datatypeMap;

template <typename T>
void registerDatatypeFor(MPI_Datatype dt) {
	datatypeMap.emplace(typeid(T), dt);
}

template <typename T>
MPI_Datatype getDatatypeFor() {
	return datatypeMap.at(typeid(T));
}

#endif //MPI_TYPEMAP_H
