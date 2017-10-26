//
// Created by blueeyedhush on 02.07.17.
//

#include <unordered_set>
#include <gtest/gtest.h>
#include <representations/ArrayBackedChunkedPartition.h>

#define TEST_NAME ArrayBackedChunkedPartition

typedef int TestLocalId;
typedef int TestNumId;
typedef ArrayBackedChunkedPartition<TestLocalId, TestNumId> G;

IMPORT_ALIASES(G)

const std::string stgPath = "resources/test/SimpleTestGraph.adjl";


TEST(TEST_NAME, LocalVertexCorrectness) {
	ABCGraphHandle<LocalId,NumericId> b0(stgPath, 2, 0, {});
	ABCGraphHandle<LocalId,NumericId> b1(stgPath, 2, 1, {});

	auto& gp0 = b0.getGraph();
	auto& gp1 = b1.getGraph();

	std::unordered_set<LocalId> actualVertexId0;
	gp0.foreachMasterVertex([&actualVertexId0](const LocalId id) {
		actualVertexId0.insert(id);
		return CONTINUE;
	});

	std::unordered_set<LocalId> actualVertexId1;
	gp1.foreachMasterVertex([&actualVertexId1](const LocalId id) {
		actualVertexId1.insert(id);
		return CONTINUE;
	});

	std::unordered_set<LocalId> expectedLocalVertices = {0,1};
	ASSERT_EQ(expectedLocalVertices, actualVertexId0);
	ASSERT_EQ(expectedLocalVertices, actualVertexId1);
}

TEST(TEST_NAME, ToMasterNodeId) {
	ABCGraphHandle<LocalId,NumericId> b0(stgPath, 2, 0, {});

	auto& gp0 = b0.getGraph();

	ASSERT_EQ((gp0.toMasterNodeId(GlobalId(0, 0))), 0);
	ASSERT_EQ((gp0.toMasterNodeId(GlobalId(1, 0))), 1);
}

TEST(TEST_NAME, ToNumeric) {
	ABCGraphHandle<LocalId,NumericId> b0(stgPath, 2, 0, {});
	ABCGraphHandle<LocalId,NumericId> b1(stgPath, 2, 1, {});

	auto& gp0 = b0.getGraph();
	auto& gp1 = b1.getGraph();

	auto gid0 = gp0.toGlobalId(0);
	auto gid1 = gp0.toGlobalId(1);
	auto gid2 = gp1.toGlobalId(0);
	auto gid3 = gp1.toGlobalId(1);

	ASSERT_EQ(gp0.toNumeric(gid0), 0);
	ASSERT_EQ(gp0.toNumeric(gid1), 1);
	ASSERT_EQ(gp1.toNumeric(gid2), 2);
	ASSERT_EQ(gp1.toNumeric(gid3), 3);
}

TEST(TEST_NAME, ForEachNeighbouringVertex) {
	ABCGraphHandle<LocalId,NumericId> b(stgPath, 2, 1, {});

	auto gp = b.getGraph();

	GlobalId gid0(0,0);
	GlobalId gid1(0,1);
	GlobalId gid2(1,0);
	GlobalId gid3(1,1);

	std::unordered_set<NumericId> expectedNeighbours = {
			gp.toNumeric(gid0),
			gp.toNumeric(gid1),
			gp.toNumeric(gid2)
	};
	std::unordered_set<NumericId> actualNeighbours;

	gp.foreachNeighbouringVertex(gid3.localId, [&actualNeighbours, &gp](const GlobalId& nid) {
		actualNeighbours.insert(gp.toNumeric(nid));
		return CONTINUE;
	});

	ASSERT_EQ(expectedNeighbours, actualNeighbours);
}

TEST(ABCPGraphBuilder, GraphBuilding) {
	auto path = std::string(stgPath);
	ABCGraphHandle<LocalId,NumericId> builder0(stgPath, 2, 0, {});

	auto gp = builder0.getGraph();
}

TEST(ABCPGraphBuilder, VertexConversion) {
	auto path = std::string(stgPath);
	ABCGraphHandle<LocalId,NumericId> builder0(stgPath, 2, 0, {0, 3});

	auto gp = builder0.getGraph();
	auto cv = builder0.getConvertedVertices();

	ASSERT_EQ(cv.size(), 2);
	ASSERT_EQ(cv[0], GlobalId(0,0));
	ASSERT_EQ(cv[1], GlobalId(1,1));
}
