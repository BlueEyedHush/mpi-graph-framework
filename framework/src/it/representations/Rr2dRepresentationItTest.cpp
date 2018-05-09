//
// Created by blueeyedhush on 14.07.17.
//

#include <representations/RoundRobin2DPartition.h>
#include "RepresentationItTest.h"

using G = RoundRobin2DPartition<TestLocalId, TestNumId>;

TEST(Rr2dRepresentation, PartitioningPreservesGraphStructure) {

	representationTest<RR2DHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank, auto vertexIds) {
				return RR2DHandle<TestLocalId, TestNumId>("resources/test/SimpleTestGraph.adjl", vertexIds);
			}, "resources/test/SimpleTestGraph");

}
