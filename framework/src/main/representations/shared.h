//
// Created by blueeyedhush on 23.11.17.
//

#ifndef FRAMEWORK_SHARED_H
#define FRAMEWORK_SHARED_H

template <typename TLocalId>
struct ALHPGlobalVertexId {
	ALHPGlobalVertexId() : ALHPGlobalVertexId(-1, 0) {}
	ALHPGlobalVertexId(NodeId nid, TLocalId lid) : nodeId(nid), localId(lid) {}

	NodeId nodeId;
	TLocalId localId;

	static MPI_Datatype mpiDatatype() {
		MPI_Datatype dt;
		int blocklengths[] = {1, 1};
		MPI_Aint displacements[] = {
				offsetof(ALHPGlobalVertexId, nodeId),
				offsetof(ALHPGlobalVertexId, localId)
		};
		MPI_Datatype building_types[] = {NODE_ID_MPI_TYPE, getDatatypeFor<TLocalId>()};
		MPI_Type_create_struct(2, blocklengths, displacements, building_types, &dt);
		return dt;
	}
};

#endif //FRAMEWORK_SHARED_H
