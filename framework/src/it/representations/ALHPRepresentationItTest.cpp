//
// Created by blueeyedhush on 14.07.17.
//

#include <representations/AdjacencyListHashPartition.h>
#include "RepresentationItTest.h"

using G = ALHPGraphPartition<TestLocalId, TestNumId>;

TEST(ALHPRepresentation, PartitioningPreservesGraphStructure) {
	std::vector<OriginalVertexId> vertexIds({0, 1, 2, 3});

	std::set<std::pair<OriginalVertexId, OriginalVertexId>> expectedEdges({
			                                                                      {0,1},{0,2},{0,3},
			                                                                      {1,0},{1,2},{1,3},
			                                                                      {2,0},{2,1},{2,3},
			                                                                      {3,0},{3,1},{3,2}
	                                                                      });

	representationTest<ALHGraphHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank) {
				return ALHGraphHandle<TestLocalId, TestNumId>("resources/test/SimpleTestGraph.adjl", vertexIds);
			}, vertexIds, expectedEdges);
}
