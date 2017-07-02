//
// Created by blueeyedhush on 02.07.17.
//

#include "CsvReader.h"

static std::vector<int> parseLine(const std::string &line) {
	std::vector<int> convertedLine;

	std::string num_str("");
	bool charInProgress = false;
	for(auto ch: line) {
		if(isdigit(ch)) {
			if (!charInProgress) {
				charInProgress = true;
			}
			num_str.push_back(ch);
		} else {
			if(charInProgress) {
				int num = std::stoi(num_str);
				convertedLine.push_back(num);
				num_str.clear();
			}
		}
	}

	if(charInProgress && num_str.size() > 0) {
		int num = std::stoi(num_str);
		convertedLine.push_back(num);
	}

	return convertedLine;
}

CsvReader::CsvReader(std::string path) {
	ifs.open(path, std::ifstream::in);
}

CsvReader::~CsvReader() {
	ifs.close();
}

boost::optional<std::vector<int>> CsvReader::getNextLine() {
	line.clear();
	std::getline(ifs, line);
	if (!line.empty()) {
		return parseLine(line);
	} else {
		return boost::none;
	}
}
