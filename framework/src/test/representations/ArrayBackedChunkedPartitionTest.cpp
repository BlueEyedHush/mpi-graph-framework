//
// Created by blueeyedhush on 02.07.17.
//

#include <representations/ArrayBackedChunkedPartition.h>
#include <unordered_set>
#include <gtest/gtest.h>


#define TEST_NAME ArrayBackedChunkedPartition

typedef int TestLocalId;
typedef int TestNumId;

const int E_N = 12;
const int V_N = 4;

int E[E_N] = {
		1, 2, 3,
		0, 2, 3,
		0, 1, 3,
		0, 1, 2
};

int V_OFFSETS[V_N] = {
		0,
		3,
		6,
		9
};

TEST(TEST_NAME, LocalVertexCorrectness) {
	ABCPGraphBuilder<TestLocalId,TestNumId,true> b0(2, 0);
	ABCPGraphBuilder<TestLocalId,TestNumId,true> b1(2, 1);

	auto gp0 = b0.buildGraph("resources/test/SimpleTestGraph.adjl", {});
	auto gp1 = b1.buildGraph("resources/test/SimpleTestGraph.adjl", {});

	std::unordered_set<LocalVertexId> actualVertexId0;
	gp0->foreachMasterVertex([&actualVertexId0](const TestLocalId id) {
		actualVertexId0.insert(id);
		return true;
	});

	std::unordered_set<LocalVertexId> actualVertexId1;
	gp1->foreachMasterVertex([&actualVertexId1](const TestLocalId id) {
		actualVertexId1.insert(id);
		return true;
	});

	std::unordered_set<LocalVertexId> expectedLocalVertices = {0,1};
	ASSERT_EQ(expectedLocalVertices, actualVertexId0);
	ASSERT_EQ(expectedLocalVertices, actualVertexId1);

	b0.destroyGraph(gp0);
	b1.destroyGraph(gp1);
}

TEST(TEST_NAME, ToMasterNodeId) {
	ABCPGraphBuilder<TestLocalId,TestNumId,true> b0(2, 0);

	auto gp0 = b0.buildGraph("resources/test/SimpleTestGraph.adjl", {});

	ASSERT_EQ(gp0->toMasterNodeId(ABCPGlobalVertexId(0, 0)), 0);
	ASSERT_EQ(gp0->toMasterNodeId(ABCPGlobalVertexId(1, 0)), 1);

	b0.destroyGraph(gp0);
}

TEST(TEST_NAME, ToNumeric) {
	ABCPGraphBuilder<TestLocalId,TestNumId,true> b0(2, 0);
	ABCPGraphBuilder<TestLocalId,TestNumId,true> b1(2, 1);

	auto* gp0 = b0.buildGraph("resources/test/SimpleTestGraph.adjl", {});
	auto* gp1 = b1.buildGraph("resources/test/SimpleTestGraph.adjl", {});

	const GlobalVertexId& gid0 = gp0->toGlobalId(0);
	const GlobalVertexId& gid1 = gp0->toGlobalId(1);
	const GlobalVertexId& gid2 = gp1->toGlobalId(0);
	const GlobalVertexId& gid3 = gp1->toGlobalId(1);

	ASSERT_EQ(gp0->toNumeric(gid0), 0);
	ASSERT_EQ(gp0->toNumeric(gid1), 1);
	ASSERT_EQ(gp1->toNumeric(gid2), 2);
	ASSERT_EQ(gp1->toNumeric(gid3), 3);

	gp0->freeGlobalId(gid0);
	gp0->freeGlobalId(gid1);
	gp1->freeGlobalId(gid2);
	gp1->freeGlobalId(gid3);

	b0.destroyGraph(gp0);
	b1.destroyGraph(gp1);
}

TEST(TEST_NAME, ForEachNeighbouringVertex) {
	ABCPGraphBuilder<TestLocalId,TestNumId,true> b(2, 1);

	auto gp = b.buildGraph("resources/test/SimpleTestGraph.adjl", {});

	ABCPGlobalVertexId gid0(0,0);
	ABCPGlobalVertexId gid1(0,1);
	ABCPGlobalVertexId gid2(1,0);
	ABCPGlobalVertexId gid3(1,1);

	std::unordered_set<TestNumId> expectedNeighbours = {
			gp->toNumeric(gid0),
			gp->toNumeric(gid1),
			gp->toNumeric(gid2)
	};
	std::unordered_set<unsigned long long> actualNeighbours;

	gp->foreachNeighbouringVertex(gid3.localId, [&actualNeighbours, &gp](const GlobalVertexId& nid) {
		actualNeighbours.insert(gp->toNumeric(nid));
		return true;
	});

	ASSERT_EQ(expectedNeighbours, actualNeighbours);

	b.destroyGraph(gp);
}

TEST(ABCPGraphBuilder, GraphBuilding) {
	auto path = std::string("resources/test/SimpleTestGraph.adjl");
	ABCPGraphBuilder builder0(2, 0);

	auto gp = builder0.buildGraph(path, {0, 3});
	auto cv = builder0.getConvertedVertices();

	ASSERT_TRUE(cv.size() == 2);

	ABCPGlobalVertexId *cv0 = reinterpret_cast<ABCPGlobalVertexId *>(cv[0]);
	ABCPGlobalVertexId *cv1 = reinterpret_cast<ABCPGlobalVertexId *>(cv[1]);

	ASSERT_EQ(*cv0, ABCPGlobalVertexId(0,0));
	ASSERT_EQ(*cv1, ABCPGlobalVertexId(1,1));

	builder0.destroyGraph(gp);
}

TEST(ABCPGraphBuilder, VertexConversion) {
	auto path = std::string("resources/test/SimpleTestGraph.adjl");
	ABCPGraphBuilder builder0(2, 0);

	auto gp = builder0.buildGraph(path, {0, 3});
	auto cv = builder0.getConvertedVertices();

	ASSERT_TRUE(cv.size() == 2);

	ABCPGlobalVertexId *cv0 = reinterpret_cast<ABCPGlobalVertexId *>(cv[0]);
	ABCPGlobalVertexId *cv1 = reinterpret_cast<ABCPGlobalVertexId *>(cv[1]);

	ASSERT_EQ(*cv0, ABCPGlobalVertexId(0,0));
	ASSERT_EQ(*cv1, ABCPGlobalVertexId(1,1));

	builder0.destroyGraph(gp);
}