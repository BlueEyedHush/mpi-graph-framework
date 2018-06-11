//
// Created by blueeyedhush on 14.07.17.
//

#include <utils/Config.h>
#include <representations/AdjacencyListHashPartition.h>
#include "RepresentationItTest.h"

using GH = ALHGraphHandle<TestLocalId, TestNumId>;
using G = ALHPGraphPartition<TestLocalId, TestNumId>;

static GBAuxiliaryParams getAuxParams() {
	ConfigMap cm;
	cm.emplace(GH::E_DIV_OPT, "1");
	cm.emplace(GH::V_DIV_OPT, "1");

	GBAuxiliaryParams auxParams;
	auxParams.configMap = cm;

	return auxParams;
}

TEST(ALHPRepresentation, PartitioningPreservesStructureSTG) {

	representationTest<GH>(
			[&](NodeId size, NodeId rank, auto vertexIds) {
				return ALHGraphHandle<TestLocalId, TestNumId>("resources/test/SimpleTestGraph.adjl",
				                                              vertexIds,
				                                              getAuxParams());
			}, "resources/test/SimpleTestGraph");

}

TEST(ALHPRepresentation, PartitioningPreservesStructurePowerlaw0) {

	representationTest<GH>(
			[&](NodeId size, NodeId rank, auto vertexIds) {
				return ALHGraphHandle<TestLocalId, TestNumId>("resources/test/powerlaw_25_2_05_876.adjl",
				                                              vertexIds,
				                                              getAuxParams());
			}, "resources/test/powerlaw_25_2_05_876");

}


