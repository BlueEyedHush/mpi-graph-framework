//
// Created by blueeyedhush on 02.07.17.
//

#include <representations/ArrayBackedChunkedPartition.h>
#include <unordered_set>
#include <gtest/gtest.h>


#define TEST_NAME ArrayBackedChunkedPartition

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
	const int P_N = 2;
	auto gp0 = ArrayBackedChunkedPartition(E_N, V_N, P_N, 0, E, V_OFFSETS);
	auto gp1 = ArrayBackedChunkedPartition(E_N, V_N, P_N, 1, E, V_OFFSETS);

	std::unordered_set<LocalVertexId> actualVertexId0;
	gp0.forEachLocalVertex([&actualVertexId0](LocalVertexId id) {
		actualVertexId0.insert(id);
	});

	std::unordered_set<LocalVertexId> actualVertexId1;
	gp1.forEachLocalVertex([&actualVertexId1](LocalVertexId id) {
		actualVertexId1.insert(id);
	});


	std::unordered_set<LocalVertexId> expectedLocalVertices = {0,1};
	ASSERT_EQ(expectedLocalVertices, actualVertexId0);
	ASSERT_EQ(expectedLocalVertices, actualVertexId1);
}

TEST(TEST_NAME, GetNodeId) {
	const int P_N = 2;
	auto gp0 = ArrayBackedChunkedPartition(E_N, V_N, P_N, 0, E, V_OFFSETS);
	auto gp1 = ArrayBackedChunkedPartition(E_N, V_N, P_N, 1, E, V_OFFSETS);

	ASSERT_EQ(gp0.getNodeId(), 0);
	ASSERT_EQ(gp1.getNodeId(), 1);
}

TEST(TEST_NAME, IsLocal) {
	const int P_N = 2;
	auto gp0 = ArrayBackedChunkedPartition(E_N, V_N, P_N, 0, E, V_OFFSETS);

	GlobalVertexId gid0;
	gid0.nodeId = 0;
	gid0.localId = 0;

	GlobalVertexId gid1;
	gid1.nodeId = 1;
	gid1.localId = 0;

	ASSERT_TRUE(gp0.isLocalVertex(gid0));
	ASSERT_FALSE(gp0.isLocalVertex(gid1));
}

TEST(TEST_NAME, GetNeighbours) {
	const int P_N = 2;
	auto gp1 = ArrayBackedChunkedPartition(E_N, V_N, P_N, 1, E, V_OFFSETS);


	GlobalVertexId gid0;
	gid0.nodeId = 0;
	gid0.localId = 0;

	GlobalVertexId gid1;
	gid1.nodeId = 0;
	gid1.localId = 1;

	GlobalVertexId gid2;
	gid2.nodeId = 1;
	gid2.localId = 0;

	GlobalVertexId gid3;
	gid3.nodeId = 1;
	gid3.localId = 1;

	std::unordered_set<unsigned long long> expectedNeighbours = {
			gp1.toNumerical(gid0),
			gp1.toNumerical(gid1),
			gp1.toNumerical(gid2)
	};
	std::unordered_set<unsigned long long> actualNeighbours;

	gp1.forEachNeighbour(gid3.localId, [&actualNeighbours, &gp1](GlobalVertexId nid) {
		actualNeighbours.insert(gp1.toNumerical(nid));
	});

	ASSERT_EQ(expectedNeighbours, actualNeighbours);
}

TEST(TEST_NAME, LoadFromFile) {
	auto path = std::string("resources/test/SimpleTestGraph.adjl");
	ABCPGraphBuilder builder(2,1);
	auto gp1 = builder.buildGraph(path);

	GlobalVertexId gid0;
	gid0.nodeId = 0;
	gid0.localId = 0;

	GlobalVertexId gid1;
	gid1.nodeId = 0;
	gid1.localId = 1;

	GlobalVertexId gid2;
	gid2.nodeId = 1;
	gid2.localId = 0;

	GlobalVertexId gid3;
	gid3.nodeId = 1;
	gid3.localId = 1;

	std::unordered_set<unsigned long long> expectedNeighbours = {
			gp1->toNumerical(gid0),
			gp1->toNumerical(gid1),
			gp1->toNumerical(gid2)
	};
	std::unordered_set<unsigned long long> actualNeighbours;

	gp1->forEachNeighbour(gid3.localId, [&actualNeighbours, gp1](GlobalVertexId nid) {
		actualNeighbours.insert(gp1->toNumerical(nid));
	});

	builder.destroyGraph(gp1);
	ASSERT_EQ(expectedNeighbours, actualNeighbours);
}

TEST(TEST_NAME, ToNumericalReturnsIndicesFromFile) {
	auto path = std::string("resources/test/SimpleTestGraph.adjl");
	ABCPGraphBuilder builder0(2, 0);
	ABCPGraphBuilder builder1(2, 1);

	auto gp0 = builder0.buildGraph(path);
	auto gp1 = builder1.buildGraph(path);

	GlobalVertexId gid0;
	gid0.nodeId = 0;
	gid0.localId = 0;

	GlobalVertexId gid1;
	gid1.nodeId = 0;
	gid1.localId = 1;

	GlobalVertexId gid2;
	gid2.nodeId = 1;
	gid2.localId = 0;

	GlobalVertexId gid3;
	gid3.nodeId = 1;
	gid3.localId = 1;

	ASSERT_EQ(gp0.toNumerical(gid0), 0);
	ASSERT_EQ(gp0.toNumerical(gid1), 1);
	ASSERT_EQ(gp1.toNumerical(gid2), 2);
	ASSERT_EQ(gp1.toNumerical(gid3), 3);

	builder0.destroyGraph(gp0);
	builder1.destroyGraph(gp1);
}