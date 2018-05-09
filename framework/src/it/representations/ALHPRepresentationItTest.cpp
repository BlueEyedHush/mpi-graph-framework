//
// Created by blueeyedhush on 14.07.17.
//

#include <representations/AdjacencyListHashPartition.h>
#include "RepresentationItTest.h"

using G = ALHPGraphPartition<TestLocalId, TestNumId>;

TEST(ALHPRepresentation, PartitioningPreservesStructureSTG) {

	representationTest<ALHGraphHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank, auto vertexIds) {
				return ALHGraphHandle<TestLocalId, TestNumId>("resources/test/SimpleTestGraph.adjl", vertexIds);
			}, "resources/test/SimpleTestGraph");

}

TEST(ALHPRepresentation, PartitioningPreservesStructurePowerlaw0) {

	representationTest<ALHGraphHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank, auto vertexIds) {
				return ALHGraphHandle<TestLocalId, TestNumId>("resources/test/powerlaw_25_2_05_876.adjl", vertexIds);
			}, "resources/test/powerlaw_25_2_05_876");

}


