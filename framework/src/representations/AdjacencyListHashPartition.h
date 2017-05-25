//
// Created by blueeyedhush on 26.05.17.
//

#ifndef FRAMEWORK_ADJACENCYLISTHASHPARTITION_H
#define FRAMEWORK_ADJACENCYLISTHASHPARTITION_H

#include <GraphBuilder.h>
#include <GraphPartition.h>

class ALHPGraphBuilder : public GraphBuilder {
	virtual GraphPartition* buildGraph(std::string path, GraphPartition *memory);
};


#endif //FRAMEWORK_ADJACENCYLISTHASHPARTITION_H
