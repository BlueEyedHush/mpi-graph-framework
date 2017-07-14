//
// Created by blueeyedhush on 14.07.17.
//

#ifndef FRAMEWORK_BSPVALIDATOR_H
#define FRAMEWORK_BSPVALIDATOR_H


#include <utility>
#include <Validator.h>

class BspValidator : public Validator<std::pair<GlobalVertexId, int>*> {
public:
	// @ToDo - (types) path length should be parametrizable + registering type with MPI
	virtual bool validate(GraphPartition *g, std::pair<GlobalVertexId, int> *partialSolution) override;
};


#endif //FRAMEWORK_BSPVALIDATOR_H
