//
// Created by blueeyedhush on 18.07.17.
//

#ifndef FRAMEWORK_BFS_H
#define FRAMEWORK_BFS_H

#include <mpi.h>
#include <utility>
#include <vector>
#include <stddef.h>
#include <glog/logging.h>

#include <Prerequisites.h>
#include <Algorithm.h>
#include <utils/VariableLengthBufferManager.h>
#include <utils/MpiTypemap.h>

/**
 * even though 1D algorithms are compatibile with common interface,
 * they might not work correctly with 2D representation
 */

template <class TGraphPartition>
class Bfs : public Algorithm<std::pair<typename TGraphPartition::GidType*, GraphDist*>*, TGraphPartition> {
protected:
	IMPORT_ALIASES(TGraphPartition)

public:
	Bfs(const GlobalId _bfsRoot) : result(nullptr, nullptr), bfsRoot(_bfsRoot) {};

	virtual std::pair<GlobalId*, GraphDist*> *getResult() override {
		return &result;
	};

	virtual ~Bfs() override {
		if(result.first != nullptr) delete[] result.first;
		if(result.second != nullptr) delete[] result.second;
	};

protected:
	std::pair<GlobalId*, GraphDist*> result;
	GlobalId& getPredecessor(LocalId vid) {
		return result.first[vid];
	}
	GraphDist& getDistance(LocalId vid) {
		return result.second[vid];
	}

	const GlobalId bfsRoot;
};



namespace details {
	namespace fixedLen {
		const int MAX_VERTICES_IN_MESSAGE = 100;

		template <typename TLocalId, typename TGlobalId>
		struct VertexMessage {
			int vidCount = 0;
			TLocalId vertexIds[MAX_VERTICES_IN_MESSAGE];
			TGlobalId predecessors[MAX_VERTICES_IN_MESSAGE];
			GraphDist distances[MAX_VERTICES_IN_MESSAGE];

			static MPI_Datatype* createVertexMessageDatatype(MPI_Datatype gidDatatype) {
				MPI_Datatype *memory = new MPI_Datatype;

				const int blocklens[] = {0, 1, MAX_VERTICES_IN_MESSAGE, MAX_VERTICES_IN_MESSAGE, MAX_VERTICES_IN_MESSAGE, 0};
				const MPI_Aint disparray[] = {
						0,
						offsetof(VertexMessage, vidCount),
						offsetof(VertexMessage, vertexIds),
						offsetof(VertexMessage, predecessors),
						offsetof(VertexMessage, distances),
						sizeof(VertexMessage),
				};
				auto localIdMpiType = datatypeMap.at(typeid(TLocalId));
				const MPI_Datatype types[] = {MPI_LB, MPI_INT, localIdMpiType, gidDatatype, GRAPH_DIST_MPI_TYPE, MPI_UB};

				MPI_Type_create_struct(6, blocklens, disparray, types, memory);
				MPI_Type_commit(memory);

				return memory;
			};

			static void cleanupMpiDatatype(MPI_Datatype* dt) {
				MPI_Type_free(dt);
				delete dt;
			}
		};
	}

	namespace varLength {
		template<typename TLocalId, typename TGlobalId>
		struct NewFrontierVertexInfo {
			TLocalId vertexId;
			TGlobalId predecessor;
			GraphDist distance;

			static MPI_Datatype* createMpiDatatype(MPI_Datatype gidDatatype) {
				MPI_Datatype *memory = new MPI_Datatype;

				const int blocklens[] = {0, 1, 1, 1, 0};
				const MPI_Aint disparray[] = {
						0,
						offsetof(NewFrontierVertexInfo, vertexId),
						offsetof(NewFrontierVertexInfo, predecessor),
						offsetof(NewFrontierVertexInfo, distance),
						sizeof(NewFrontierVertexInfo),
				};
				auto localIdMpiType = datatypeMap.at(typeid(TLocalId));
				const MPI_Datatype types[] = {MPI_LB, localIdMpiType, gidDatatype, GRAPH_DIST_MPI_TYPE, MPI_UB};

				MPI_Type_create_struct(5, blocklens, disparray, types, memory);
				MPI_Type_commit(memory);

				return memory;
			};

			static void cleanupMpiDatatype(MPI_Datatype* dt) {
				MPI_Type_free(dt);
				delete dt;
			}
		};
	}
}

/*
#include "bfs/BfsFixedMessage.h"
#include "bfs/BfsVarMessage.h"
#include "bfs/Bfs1CommsRound.h"
*/

#endif //FRAMEWORK_BFS_H
