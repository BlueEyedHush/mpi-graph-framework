//
// Created by blueeyedhush on 02.07.17.
//

#include <utils/AdjacencyListReader.h>
#include <gtest/gtest.h>

std::string path = "resources/test/SimpleTestGraph.adjl";

TEST(AdjacencyListReader, VertexEdgeCount) {
	AdjacencyListReader reader(path);

	ASSERT_EQ(reader.getEdgeCount(), 12);
	ASSERT_EQ(reader.getVertexCount(), 4);
}

TEST(AdjacencyListReader, Structure) {
	AdjacencyListReader reader(path);

	std::vector<VertexSpec> expected = {
		VertexSpec(0, {1, 2, 3}),
		VertexSpec(1, {0, 2, 3}),
		VertexSpec(2, {0, 1, 3}),
		VertexSpec(3, {0, 1, 2}),
	};

	std::vector<VertexSpec> actual;
	while(boost::optional<VertexSpec> oVertexSpec = reader.getNextVertex()) {
		actual.push_back(*oVertexSpec);
	}

	ASSERT_EQ(actual, expected);
}