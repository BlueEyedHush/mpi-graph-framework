//
// Created by blueeyedhush on 17.10.17.
//

#include "MpiTypemap.h"

typedef std::unordered_map<std::type_index, MPI_Datatype> TypeMap;

TypeMap mapWithCommonTypes() {
	TypeMap m;

	m.emplace(typeid(short), MPI_SHORT);
	m.emplace(typeid(unsigned short), MPI_UNSIGNED_SHORT);
	m.emplace(typeid(int), MPI_INT);
	m.emplace(typeid(unsigned int), MPI_UNSIGNED);
	m.emplace(typeid(long long), MPI_LONG_LONG_INT);
	m.emplace(typeid(unsigned long long), MPI_UNSIGNED_LONG_LONG);

	return m;
}

TypeMap datatypeMap = mapWithCommonTypes();
