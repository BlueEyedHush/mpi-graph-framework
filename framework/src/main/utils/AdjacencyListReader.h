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
	AdjacencyListReader(std::string path)
			: csvReader(path), initialized(false), vertexCount(0), edgeCount(0) {}
	
	size_t getVertexCount() {
		if(!initialized) initialize();
		return vertexCount;
	}
	size_t getEdgeCount() {
		if(!initialized) initialize();
		return edgeCount;
	}

	boost::optional<VertexSpec<TVertexId>> getNextVertex() {
		auto oParsedLine = csvReader.getNextLine();
		if(oParsedLine != boost::none) {
			auto parsedLine = *oParsedLine;
			TVertexId vid = parsedLine[0];
			auto neighStartIt = ++(parsedLine.begin());
			auto neighEndIt = parsedLine.end();
			return VertexSpec<TVertexId>(vid, std::set<TVertexId>(neighStartIt, neighEndIt));
		} else {
			return boost::none;
		}
	}

private:
	CsvReader<TVertexId> csvReader;
	size_t vertexCount;
	size_t edgeCount;
	bool initialized;

	void initialize() {
		auto vertexLine = *csvReader.getNextLine();
		vertexCount = vertexLine[0];
		auto edgeLine = *csvReader.getNextLine();
		edgeCount = edgeLine[0];
	}
};


#endif //FRAMEWORK_ADJACENCYLISTREADER_H
