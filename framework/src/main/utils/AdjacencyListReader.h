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

template <typename TVertexId>
struct VertexSpec {
	VertexSpec(TVertexId _vertexId, std::set<TVertexId> _neighbours) : vertexId(_vertexId), neighbours(_neighbours) {}

	TVertexId vertexId;
	std::set<TVertexId> neighbours;

	bool operator ==(const VertexSpec &o) const {
		return (this->vertexId == o.vertexId) && (this->neighbours == o.neighbours);
	};
};

template <typename TVertexId>
class AdjacencyListReader {
public:
	AdjacencyListReader(std::string path) : csvReader(path) {
		auto vertexLine = *csvReader.getNextLine();
		vertexCount = vertexLine[0];
		auto edgeLine = *csvReader.getNextLine();
		edgeCount = edgeLine[0];
	}
	
	size_t getVertexCount() {return vertexCount;}
	size_t getEdgeCount() {return edgeCount;}

	boost::optional<VertexSpec> getNextVertex() {
		auto oParsedLine = csvReader.getNextLine();
		if(oParsedLine != boost::none) {
			auto parsedLine = *oParsedLine;
			TVertexId vid = parsedLine[0];
			auto neighStartIt = ++(parsedLine.begin());
			auto neighEndIt = parsedLine.end();
			return VertexSpec(vid, std::set<TVertexId>(neighStartIt, neighEndIt));
		} else {
			return boost::none;
		}
	}

private:
	CsvReader<TVertexId> csvReader;
	size_t vertexCount;
	size_t edgeCount;
};


#endif //FRAMEWORK_ADJACENCYLISTREADER_H
