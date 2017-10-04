//
// Created by blueeyedhush on 18.07.17.
//

#ifndef FRAMEWORK_BFS_H
#define FRAMEWORK_BFS_H

#include <mpi.h>
#include <utility>
#include <Algorithm.h>

typedef int GraphDist;
#define GRAPH_DIST_MPI_TYPE MPI_INT

class Bfs : public Algorithm<std::pair<GlobalVertexId*, int*>*> {
public:
	Bfs(GlobalVertexId _bfsRoot);
	virtual std::pair<GlobalVertexId*, int*> *getResult() override;
	virtual ~Bfs() override;

protected:
	std::pair<GlobalVertexId*, int*> result;
	GlobalVertexId& getPredecessor(int vid) {
		return result.first[vid];
	}
	int& getDistance(int vid) {
		return result.second[vid];
	}

	const GlobalVertexId bfsRoot;
};

class Bfs_Mp_FixedMsgLen_1D_2CommRounds : public Bfs {
public:
	const static int MAX_VERTICES_IN_MESSAGE = 100;
	const static int SEND_TAG = 1;

	Bfs_Mp_FixedMsgLen_1D_2CommRounds(GlobalVertexId _bfsRoot);
	virtual ~Bfs_Mp_FixedMsgLen_1D_2CommRounds();
	virtual bool run(GraphPartition *g) override;

private:
	struct VertexMessage {
		int vidCount = 0;
		LocalVertexId vertexIds[MAX_VERTICES_IN_MESSAGE];
		LocalVertexId predecessors[MAX_VERTICES_IN_MESSAGE];
		GraphDist distances[MAX_VERTICES_IN_MESSAGE];
	};

	void createVertexMessageDatatype(MPI_Datatype *memory);
};

class Bfs_Mp_VarMsgLen_1D_2CommRounds : public Bfs {
public:
	const static int SEND_TAG = 1;

	Bfs_Mp_VarMsgLen_1D_2CommRounds(GlobalVertexId _bfsRoot);
	virtual ~Bfs_Mp_VarMsgLen_1D_2CommRounds();
	virtual bool run(GraphPartition *g) override;

private:
	struct VertexMessage {
		LocalVertexId vertexId;
		LocalVertexId predecessor;
		GraphDist distance;
	};

	void createVertexMessageDatatype(MPI_Datatype *memory);
};

#endif //FRAMEWORK_BFS_H
