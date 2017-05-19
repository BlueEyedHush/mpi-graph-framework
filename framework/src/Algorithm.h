//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_ALGORITHM_H
#define FRAMEWORK_ALGORITHM_H

#include "GraphPartition.h"

class Algorithm {
public:
	virtual bool run(GraphPartition *g) = 0;
};

#endif //FRAMEWORK_ALGORITHM_H
