//
// Created by blueeyedhush on 02.07.17.
//

#ifndef FRAMEWORK_ADJACENCYLISTREADER_H
#define FRAMEWORK_ADJACENCYLISTREADER_H

#include <string>
#include <vector>
#include <set>

struct VertexSpec {
	int vertexId;
	std::set<int> neighbours;
};

class AdjacencyListReader {
public:
	AdjacencyListReader(std::string path);

	int getVertexCount();
	int getEdgeCount();

	bool hasNextVertex();
	VertexSpec getNextVertex();
};


#endif //FRAMEWORK_ADJACENCYLISTREADER_H
