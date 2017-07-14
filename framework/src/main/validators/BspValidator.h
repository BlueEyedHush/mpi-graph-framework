//
// Created by blueeyedhush on 14.07.17.
//

#ifndef FRAMEWORK_BSPVALIDATOR_H
#define FRAMEWORK_BSPVALIDATOR_H


#include <Validator.h>

class BspValidator : public Validator<GlobalVertexId*> {
public:
	virtual bool validate(GraphPartition *g, GlobalVertexId *partialSolution) override;
};


#endif //FRAMEWORK_BSPVALIDATOR_H
