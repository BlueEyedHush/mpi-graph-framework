//
// Created by blueeyedhush on 02.07.17.
//

#include "AdjacencyListReader.h"

static std::vector<int> parseLine(const std::string &line) {
	std::vector<int> convertedLine;

	std::string num_str("");
	bool charInProgress = false;
	for(auto ch: line) {
		if(isdigit(ch)) {
			if (!charInProgress) {
				charInProgress = true;
			}
			num_str.push_back(ch);
		} else {
			if(charInProgress) {
				int num = std::stoi(num_str);
				convertedLine.push_back(num);
				num_str.clear();
			}
		}
	}

	if(charInProgress && num_str.size() > 0) {
		int num = std::stoi(num_str);
		convertedLine.push_back(num);
	}

	return convertedLine;
}

AdjacencyListReader::AdjacencyListReader(std::string path) {
	ifs.open(path, std::ifstream::in);

	loadNextLine();
	vertexCount = parsedLine[0];
	loadNextLine();
	edgeCount = parsedLine[0];
}

int AdjacencyListReader::getVertexCount() {
	return vertexCount;
}

int AdjacencyListReader::getEdgeCount() {
	return edgeCount;
}

boost::optional<VertexSpec> AdjacencyListReader::getNextVertex() {
	loadNextLine();
	if(!parsedLine.empty()) {
		int vid = parsedLine[0];
		auto neighStartIt = ++(parsedLine.begin());
		auto neighEndIt = parsedLine.end();
		return VertexSpec(vid, std::set<int>(neighStartIt, neighEndIt));
	} else {
		return boost::none;
	}
}

AdjacencyListReader::~AdjacencyListReader() {
	ifs.close();
}

void AdjacencyListReader::loadNextLine() {
	line.clear();
	std::getline(ifs, line);
	if (!line.empty()) {
		parsedLine = parseLine(line);
	} else {
		parsedLine.clear();
	}
}
