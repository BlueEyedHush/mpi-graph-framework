//
// Created by blueeyedhush on 06.06.18.
//

#ifndef FRAMEWORK_CONFIG_H
#define FRAMEWORK_CONFIG_H

#include <unordered_map>
#include <string>

using ConfigMap = std::unordered_map<std::string, std::string>;

ConfigMap parseCli(const int argc, const char** argv);
std::string configurationToString(ConfigMap& cmap);

#endif //FRAMEWORK_CONFIG_H
