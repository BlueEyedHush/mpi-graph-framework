//
// Created by blueeyedhush on 25.05.17.
//

#ifndef FRAMEWORK_GRAPHBUILDER_H
#define FRAMEWORK_GRAPHBUILDER_H

#include <string>
#include "GraphPartition.h"

class GraphBuilder {
	virtual GraphPartition* buildGraph(std::string path, GraphPartition *memory) = 0;
};

#endif //FRAMEWORK_GRAPHBUILDER_H
