//
// Created by blueeyedhush on 18.07.17.
//

#ifndef FRAMEWORK_BSP_H
#define FRAMEWORK_BSP_H

#include <mpi.h>
#include <utility>
#include <Algorithm.h>

typedef int GraphDist;
#define GRAPH_DIST_MPI_TYPE MPI_INT

class Bsp_Mp_FixedMessageSize_1D_2CommRounds : public Algorithm<std::pair<GlobalVertexId*, int*>*> {
public:
	const static int MAX_VERTICES_IN_MESSAGE = 100;
	const static int SEND_TAG = 1;

	Bsp_Mp_FixedMessageSize_1D_2CommRounds(GlobalVertexId _bspRoot);
	virtual bool run(GraphPartition *g) override;
	virtual std::pair<GlobalVertexId*, int*> *getResult() override;
	virtual ~Bsp_Mp_FixedMessageSize_1D_2CommRounds() override;

private:
	struct VertexMessage {
		int vidCount = 0;
		LocalVertexId vertexIds[MAX_VERTICES_IN_MESSAGE];
		LocalVertexId predecessors[MAX_VERTICES_IN_MESSAGE];
		GraphDist distances[MAX_VERTICES_IN_MESSAGE];
	};

	void createVertexMessageDatatype(MPI_Datatype *memory);

	std::pair<GlobalVertexId*, int*> result;
	GlobalVertexId& getPredecessor(int vid) {
		return result.first[vid];
	}
	int& getDistance(int vid) {
		return result.second[vid];
	}

	const GlobalVertexId bspRoot;
};


#endif //FRAMEWORK_BSP_H
