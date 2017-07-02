//
// Created by blueeyedhush on 02.07.17.
//

#include "AdjacencyListReader.h"

AdjacencyListReader::AdjacencyListReader(std::string path) : csvReader(path) {
	auto vertexLine = *csvReader.getNextLine();
	vertexCount = vertexLine[0];
	auto edgeLine = *csvReader.getNextLine();
	edgeCount = edgeLine[0];
}

int AdjacencyListReader::getVertexCount() {
	return vertexCount;
}

int AdjacencyListReader::getEdgeCount() {
	return edgeCount;
}

boost::optional<VertexSpec> AdjacencyListReader::getNextVertex() {
	auto oParsedLine = csvReader.getNextLine();
	if(oParsedLine != boost::none) {
		auto parsedLine = *oParsedLine;
		int vid = parsedLine[0];
		auto neighStartIt = ++(parsedLine.begin());
		auto neighEndIt = parsedLine.end();
		return VertexSpec(vid, std::set<int>(neighStartIt, neighEndIt));
	} else {
		return boost::none;
	}
}

AdjacencyListReader::~AdjacencyListReader() {

}
