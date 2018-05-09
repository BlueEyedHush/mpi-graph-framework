//
// Created by blueeyedhush on 14.07.17.
//

#include <representations/RoundRobin2DPartition.h>
#include "RepresentationItTest.h"

using G = RoundRobin2DPartition<TestLocalId, TestNumId>;

TEST(Rr2dRepresentation, PartitioningPreservesGraphStructure) {
	std::vector<OriginalVertexId> vertexIds({0, 1, 2, 3});

	representationTest<RR2DHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank) {
				return RR2DHandle<TestLocalId, TestNumId>("resources/test/SimpleTestGraph.adjl", vertexIds);
			}, vertexIds, loadEdgeListFromFile("resources/test/SimpleTestGraph.el"));
}
