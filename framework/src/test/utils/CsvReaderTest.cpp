//
// Created by blueeyedhush on 02.07.17.
//

#include <utils/CsvReader.h>
#include <gtest/gtest.h>

TEST(CsvReader, LoadingFromFile) {
	CsvReader reader(std::string("resources/test/ReaderTest.csv"));

	std::vector<std::vector<int>> expected = {
		{5, 4, 3, 2, 1},
		{50, 4, 30, 2, 10}
	};

	std::vector<std::vector<int>> actual;
	while(boost::optional<std::vector<int>> parsedLine = reader.getNextLine()) {
		actual.push_back(*parsedLine);
	}

	ASSERT_EQ(actual, expected);
}