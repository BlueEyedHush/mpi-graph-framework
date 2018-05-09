//
// Created by blueeyedhush on 14.07.17.
//

#include <representations/RoundRobin2DPartition.h>
#include "RepresentationItTest.h"

using G = RoundRobin2DPartition<TestLocalId, TestNumId>;

TEST(Rr2dRepresentation, PartitioningPreservesStructureSTG) {

	representationTest<RR2DHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank, auto vertexIds) {
				return RR2DHandle<TestLocalId, TestNumId>("resources/test/SimpleTestGraph.adjl", vertexIds);
			}, "resources/test/SimpleTestGraph");

}

TEST(Rr2dRepresentation, PartitioningPreservesStructurePowerlaw0) {

	representationTest<RR2DHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank, auto vertexIds) {
				return RR2DHandle<TestLocalId, TestNumId>("resources/test/powerlaw_25_2_05_876.adjl", vertexIds);
			}, "resources/test/powerlaw_25_2_05_876");

}
