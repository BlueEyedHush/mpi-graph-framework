//
// Created by blueeyedhush on 22.11.17.
//

#ifndef FRAMEWORK_EXECUTOR_H
#define FRAMEWORK_EXECUTOR_H

#include <string>
#include <unordered_map>
#include <functional>
#include "Assembly.h"

class Executor {
	static void defaultCleaner(Assembly* assembly) {delete assembly;}

public:
	Executor(std::function<void(Assembly*)> assemblyCleaner = defaultCleaner);
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
};


#endif //FRAMEWORK_EXECUTOR_H
