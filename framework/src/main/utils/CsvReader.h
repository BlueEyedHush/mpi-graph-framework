//
// Created by blueeyedhush on 02.07.17.
//

#ifndef FRAMEWORK_CSVREADER_H
#define FRAMEWORK_CSVREADER_H

#include <string>
#include <vector>
#include <fstream>
#include <boost/optional.hpp>

// details declarations
namespace details {
	template <typename T>
	std::vector<T> parseLine(const std::string &line);
}

template <typename T>
class CsvReader {
public:
	CsvReader(std::string path) {
		ifs.open(path, std::ifstream::in);
		if (ifs.fail())
			throw std::runtime_error("Failed to open " + path + " (probably doesn't exist)");
	}

	~CsvReader() {
		ifs.close();
	}

	boost::optional<std::vector<T>> getNextLine() {
		line.clear();
		std::getline(ifs, line);
		if (!line.empty()) {
			return details::parseLine<T>(line);
		} else {
			return boost::none;
		}
	}

private:
	std::ifstream ifs;
	std::string line;
};

// details definitions
namespace details {
	template <typename T>
	std::vector<T> parseLine(const std::string &line) {
		std::vector<T> convertedLine;

		std::string num_str("");
		bool charInProgress = false;
		for(auto ch: line) {
			if(isdigit(ch) || ch == '-') {
				if (!charInProgress) {
					charInProgress = true;
				}
				num_str.push_back(ch);
			} else {
				if(charInProgress) {
					T num = static_cast<T>(std::stoull(num_str));
					convertedLine.push_back(num);
					num_str.clear();
				}
			}
		}

		if(charInProgress && num_str.size() > 0) {
			T num = static_cast<T>(std::stoull(num_str));
			convertedLine.push_back(num);
		}

		return convertedLine;
	}
}

#endif //FRAMEWORK_CSVREADER_H
