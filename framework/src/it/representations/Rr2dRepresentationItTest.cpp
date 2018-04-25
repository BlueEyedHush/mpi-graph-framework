//
// Created by blueeyedhush on 14.07.17.
//

#include <representations/RoundRobin2DPartition.h>
#include "RepresentationItTest.h"

using G = RoundRobin2DPartition<TestLocalId, TestNumId>;

TEST(Rr2dRepresentation, PartitioningPreservesGraphStructure) {
	std::vector<OriginalVertexId> vertexIds({0, 1, 2, 3});

	std::set<std::pair<OriginalVertexId, OriginalVertexId>> expectedEdges({
			                                                                      {0,1},{0,2},{0,3},
			                                                                      {1,0},{1,2},{1,3},
			                                                                      {2,0},{2,1},{2,3},
			                                                                      {3,0},{3,1},{3,2}
	                                                                      });

	representationTest<RR2DHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank) {
				return RR2DHandle<TestLocalId, TestNumId>("resources/test/SimpleTestGraph.adjl", vertexIds);
			}, vertexIds, expectedEdges);
}
