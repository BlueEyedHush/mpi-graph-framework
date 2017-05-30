//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_ALGORITHM_H
#define FRAMEWORK_ALGORITHM_H

#include "GraphPartition.h"

template <class T> class Algorithm {
public:
	virtual bool run(GraphPartition *g) = 0;
	/**
	 *  Returned result should be cleaned on object destruction
	 */
	virtual T getResult() = 0;
	virtual ~Algorithm() {};
};

#endif //FRAMEWORK_ALGORITHM_H
