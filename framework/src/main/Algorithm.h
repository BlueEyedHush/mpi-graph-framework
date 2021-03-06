//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_ALGORITHM_H
#define FRAMEWORK_ALGORITHM_H

#include <utils/Config.h>
#include "GraphPartition.h"

struct AAuxiliaryParams {
	ConfigMap config = ConfigMap();
};

template <class TResult, class TGraphPartition>
class Algorithm {
public:
	using ResultType = TResult;

	virtual bool run(TGraphPartition *g, AAuxiliaryParams aParams) = 0;
	/**
	 *  This method should only return part of the result that is local to the node
	 *  If the result is represented by array with 1:1 vertex-result mapping, it should allocate
	 *   g->getMaxLocalVertexCount(), even if number of vertices assigned to current partition is smaller
	 *
	 *  Returned result should be cleaned on object destruction
	 *
	 *  Don't use types for which you cannot create matching MPI Derived Type - it'll make creation of validator harder
	 *
	 */
	virtual TResult getResult() = 0;
	virtual ~Algorithm() {};
};

#endif //FRAMEWORK_ALGORITHM_H
