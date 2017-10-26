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
			destroyer(graph);
			graph = nullptr;
			convertedVertices.clear();

			initialized = false;
		}
	}

	virtual ~GraphPartitionHandle() {
		releaseGraph();
	};

protected:
	using GraphDestroyer = void(*)(TGraphPartition*);

	/**
	 *
	 * @param verticesToConvert
	 * @param destroyer I started with virtual inheritance, but switched to composition - virtual functions behave
	 * rather weiredly when called from destructor (which makes sense - ordinary mechanism would dispatch it on
	 * partially destructed object /since child destructors has already been executed/). This means that destroyer
	 * should not use handler object - static function'd be best.
	 */
	GraphPartitionHandle(std::vector<OriginalVertexId> verticesToConvert, GraphDestroyer destroyer)
			: verticesToConvert(verticesToConvert), initialized(false), graph(nullptr), destroyer(destroyer) {};

	virtual std::pair<TGraphPartition*, std::vector<GlobalId>> buildGraph(std::vector<OriginalVertexId> verticesToConvert) = 0;

private:
	const std::vector<OriginalVertexId> verticesToConvert;
	bool initialized;
	TGraphPartition* graph;
	std::vector<GlobalId> convertedVertices;
	GraphDestroyer destroyer;

	void initialize() {
		std::tie(graph, convertedVertices) = buildGraph(verticesToConvert);
		initialized = true;
	}
};

#endif //FRAMEWORK_GRAPHBUILDER_H
