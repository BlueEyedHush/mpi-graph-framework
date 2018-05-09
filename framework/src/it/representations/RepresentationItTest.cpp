
#include "RepresentationItTest.h"
#include <utils/CsvReader.h>

std::vector<OriginalVertexId> loadVertexListFromFile(std::string path) {
	CsvReader<OriginalVertexId> csvReader(path);
	auto line = csvReader.getNextLine();
	assert(line.is_initialized());
	return *line;
}

std::set<std::pair<OriginalVertexId, OriginalVertexId>> loadEdgeListFromFile(std::string path) {
	CsvReader<OriginalVertexId> csvReader(path);

	std::set<std::pair<OriginalVertexId, OriginalVertexId>> edgesSet;
	while(auto optionalLine = csvReader.getNextLine()) {
		std::vector<OriginalVertexId> line = *optionalLine;
		assert(line.size() == 2);
		edgesSet.emplace(std::make_pair(line[0], line[1]));
	}

	return edgesSet;
}

