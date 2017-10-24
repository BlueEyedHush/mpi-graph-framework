//
// Created by blueeyedhush on 30.05.17.
//

#ifndef FRAMEWORK_VALIDATOR_H
#define FRAMEWORK_VALIDATOR_H

#include "GraphPartition.h"

template <class TGraphPartition, class TResult>
class Validator {
public:
	using ResultType = TResult;

	bool validate(TGraphPartition *g, TResult partialSolution);
};

#endif //FRAMEWORK_VALIDATOR_H
