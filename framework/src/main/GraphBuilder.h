//
// Created by blueeyedhush on 25.05.17.
//

#ifndef FRAMEWORK_GRAPHBUILDER_H
#define FRAMEWORK_GRAPHBUILDER_H

#include <string>
#include <vector>
#include <type_traits>
#include <Prerequisites.h>
#include "GraphPartition.h"

template<typename TGraphPartition>
class GraphBuilder {
protected:
	IMPORT_ALIASES(TGraphPartition)
	
public:
	using GPType = TGraphPartition;

	TGraphPartition *buildGraph(std::string path,
	                      std::vector<OriginalVertexId> verticesToConvert);
	std::vector<GlobalId> getConvertedVertices();
	void destroyGraph(TGraphPartition*);

protected:
	GraphBuilder() {};
	~GraphBuilder() {};
};

#endif //FRAMEWORK_GRAPHBUILDER_H
