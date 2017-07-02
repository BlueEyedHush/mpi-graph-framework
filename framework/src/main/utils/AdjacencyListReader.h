//
// Created by blueeyedhush on 02.07.17.
//

#ifndef FRAMEWORK_ADJACENCYLISTREADER_H
#define FRAMEWORK_ADJACENCYLISTREADER_H

#include <string>
#include <vector>
#include <set>
#include <boost/optional.hpp>
#include <utils/CsvReader.h>

struct VertexSpec {
	VertexSpec(int _vertexId, std::set<int> _neighbours) : vertexId(_vertexId), neighbours(_neighbours) {}

	int vertexId;
	std::set<int> neighbours;

	bool operator ==(const VertexSpec &o) const {
		return (this->vertexId == o.vertexId) && (this->neighbours == o.neighbours);
	};
};

class AdjacencyListReader {
public:
	AdjacencyListReader(std::string path);
	~AdjacencyListReader();

	int getVertexCount();
	int getEdgeCount();

	boost::optional<VertexSpec> getNextVertex();

private:
	CsvReader csvReader;
	int vertexCount;
	int edgeCount;
};


#endif //FRAMEWORK_ADJACENCYLISTREADER_H
