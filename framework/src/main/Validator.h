//
// Created by blueeyedhush on 30.05.17.
//

#ifndef FRAMEWORK_VALIDATOR_H
#define FRAMEWORK_VALIDATOR_H

#include "GraphPartition.h"

template <class TGraphPartition, class TResult>
class Validator {
public:
	virtual bool validate(TGraphPartition *g, TResult partialSolution) = 0;
};

#endif //FRAMEWORK_VALIDATOR_H
