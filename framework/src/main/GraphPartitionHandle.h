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

/**
 * Responsible for buildling graph and ensuring, that only one instance is ever constructed (and can be resued between
 * different algorithms)
 *
 * @tparam TGraphPartition
 */
template<typename TGraphPartition>
class GraphPartitionHandle {
protected:
	IMPORT_ALIASES(TGraphPartition)
	
public:
	using GPType = TGraphPartition;

	TGraphPartition& getGraph() {
		if(!initialized) initialize();
		return *graph;
	}

	std::vector<GlobalId> getConvertedVertices() {
		if(!initialized) initialize();
		return convertedVertices;
	}

	/**
	 * Destructor cleans up after graph anyway - this is only used to release resources earlier
	 */
	void releaseGraph() {
		if(initialized) {
			destroyGraph(graph);
			graph = nullptr;
			convertedVertices.clear();

			initialized = false;
		}
	}

	virtual ~GraphPartitionHandle() {
		releaseGraph();
	};

protected:
	/* to prevent direct instantiation of this class */
	GraphPartitionHandle(std::vector<OriginalVertexId> verticesToConvert)
			: verticesToConvert(verticesToConvert), initialized(false), graph(nullptr) {};

	/* for concrete subclasses to implement */
	virtual std::pair<TGraphPartition*, std::vector<GlobalId>> buildGraph(std::vector<OriginalVertexId> verticesToConvert) = 0;
	virtual void destroyGraph(TGraphPartition*) = 0;

private:
	const std::vector<OriginalVertexId> verticesToConvert;
	bool initialized;
	TGraphPartition* graph;
	std::vector<GlobalId> convertedVertices;

	void initialize() {
		std::tie(graph, convertedVertices) = buildGraph(verticesToConvert);
		initialized = true;
	}
};

#endif //FRAMEWORK_GRAPHBUILDER_H
