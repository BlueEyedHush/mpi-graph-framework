//
// Created by blueeyedhush on 06.06.18.
//

#include <boost/format.hpp>
#include "Config.h"

ConfigMap parseCli(const int argc, const char** argv) {
	if (argc % 2 != 1)
		throw "Even number of CLI arguments expected (-option value)";

	ConfigMap cmap;
	for(int i = 1; i < argc; i += 2) {
		std::string option(argv[i]);
		if (option[0] != '-' || option.size() < 2)
			throw (boost::format("Argument %1%: excpected option, got '%2%'") % i % option).str();
		auto optionName = option.substr(1);

		std::string value(argv[i+1]);
		if (value[0] == '-')
			throw (boost::format("Argument %1%: excpected value, got '%2%'") % i % value).str();

		cmap.emplace(option, value);
	}

	return cmap;
}