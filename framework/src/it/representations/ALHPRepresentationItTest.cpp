//
// Created by blueeyedhush on 14.07.17.
//

#include <representations/AdjacencyListHashPartition.h>
#include "RepresentationItTest.h"

using G = ALHPGraphPartition<TestLocalId, TestNumId>;

TEST(ALHPRepresentation, PartitioningPreservesGraphStructure) {

	representationTest<ALHGraphHandle<TestLocalId, TestNumId>>(
			[&](NodeId size, NodeId rank, auto vertexIds) {
				return ALHGraphHandle<TestLocalId, TestNumId>("resources/test/SimpleTestGraph.adjl", vertexIds);
			}, "resources/test/SimpleTestGraph");
	
}
