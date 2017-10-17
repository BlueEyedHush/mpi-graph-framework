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

template<typename TGraphPartition, bool polymorphic>
class GraphBuilder {
protected:
	typedef std::conditional<polymorphic, DGraphPartition, TGraphPartition>::type G;

public:
	virtual G *buildGraph(std::string path,
	                      std::vector<OriginalVertexId> verticesToConvert) = 0;
	virtual std::vector<GlobalVertexId *> getConvertedVertices() = 0;
	/**
	 * It's better to not invoke this directly - if you ever pass received vertex to the algorithm and then
	 * call this method, serious issues can occur (e.g. because algorithm has ended, but reference to that vertex
	 * has been returned as a solution)
	 */
	virtual void destroyConvertedVertices() = 0;
	virtual void destroyGraph(const G *) = 0;
	virtual ~GraphBuilder() {};
};

#endif //FRAMEWORK_GRAPHBUILDER_H
