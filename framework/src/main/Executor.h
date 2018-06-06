//
// Created by blueeyedhush on 22.11.17.
//

#ifndef FRAMEWORK_EXECUTOR_H
#define FRAMEWORK_EXECUTOR_H

#include <string>
#include <unordered_map>
#include <functional>
#include <utils/Config.h>
#include "Assembly.h"

using AssemblyCleaner = std::function<void(Assembly*)>;

class Executor {
	static void defaultCleaner(Assembly* assembly) {delete assembly;}

public:
	Executor(ConfigMap configuration = ConfigMap(), AssemblyCleaner assemblyCleaner = defaultCleaner);
	~Executor();

	/**
	 * Takes ownership over the created assembly
	 *
	 * @param key
	 * @param assembly
	 */
	void registerAssembly(const std::string key, Assembly* assembly);
	bool executeAssembly(const std::string key);
private:
	std::unordered_map<std::string, Assembly*> assemblies;
	AssemblyCleaner assemblyCleaner;
	ConfigMap configuration;
};


#endif //FRAMEWORK_EXECUTOR_H
