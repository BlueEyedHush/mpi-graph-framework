//
// Created by blueeyedhush on 25.05.17.
//

#ifndef FRAMEWORK_GRAPHBUILDER_H
#define FRAMEWORK_GRAPHBUILDER_H

#include <string>
#include <vector>
#include "GraphPartition.h"

typedef unsigned long long OriginalVertexId;

class GraphBuilder {
public:
	virtual GraphPartition* buildGraph(std::string path,
	                                   std::vector<OriginalVertexId> verticesToConvert) = 0;
	virtual std::vector<GlobalVertexId*> getConvertedVertices() = 0;
	virtual void destroyConvertedVertices() = 0;
	virtual void destroyGraph(const GraphPartition*) = 0;
	virtual ~GraphBuilder() {};
};

#endif //FRAMEWORK_GRAPHBUILDER_H
