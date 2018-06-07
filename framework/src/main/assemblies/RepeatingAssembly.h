//
// Created by blueeyedhush on 07.06.18.
//

#ifndef FRAMEWORK_REPEATINGASSEMBLY_H
#define FRAMEWORK_REPEATINGASSEMBLY_H

#include <Assembly.h>
#include <Executor.h>

class RepeatingAssembly : public Assembly {
public:
	void doRun(ConfigMap cmap) override {
		std::string strN = cmap["ra-n"];
		int n = std::stoi(strN);
		std::string innerAssemblyName = cmap["ra-name"];

		for(int i = 0; i < n; i++) {
			parentExecutor->executeAssembly(innerAssemblyName);
		}
	};
};

#endif //FRAMEWORK_REPEATINGASSEMBLY_H
