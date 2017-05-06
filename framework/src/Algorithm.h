//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_ALGORITHM_H
#define FRAMEWORK_ALGORITHM_H

#include "Graph.h"

class Algorithm {
public:
	virtual bool run(Graph *g) = 0;
};

#endif //FRAMEWORK_ALGORITHM_H
