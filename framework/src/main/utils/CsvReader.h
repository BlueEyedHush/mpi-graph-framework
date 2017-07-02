//
// Created by blueeyedhush on 02.07.17.
//

#ifndef FRAMEWORK_CSVREADER_H
#define FRAMEWORK_CSVREADER_H

#include <string>
#include <vector>
#include <boost/optional.hpp>

class CsvReader {
public:
	CsvReader(std::string path);
	~CsvReader();

	boost::optional<std::vector<int>> getNextLine();
};


#endif //FRAMEWORK_CSVREADER_H
