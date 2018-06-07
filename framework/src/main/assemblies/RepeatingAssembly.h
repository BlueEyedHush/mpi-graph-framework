//
// Created by blueeyedhush on 07.06.18.
//

#ifndef FRAMEWORK_REPEATINGASSEMBLY_H
#define FRAMEWORK_REPEATINGASSEMBLY_H

#include <Assembly.h>

class RepeatingAssembly : public Assembly {
public:
	RepeatingAssembly(Assembly *innerAssembly) : innerAssembly(innerAssembly) {}

	void run(ConfigMap cmap) override {
		int n = std::stoi(cmap["n"]);

		for(int i = 0; i < n; i++) {
			innerAssembly->run(cmap);
		}
	};

private:
	Assembly *innerAssembly;
};

#endif //FRAMEWORK_REPEATINGASSEMBLY_H
