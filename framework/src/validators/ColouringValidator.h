//
// Created by blueeyedhush on 30.05.17.
//

#ifndef FRAMEWORK_COLOURINGVALIDATOR_H
#define FRAMEWORK_COLOURINGVALIDATOR_H

#include <Validator.h>

class ColouringValidator : public Validator<int*> {
	virtual bool validate(GraphPartition *g, int *partialSolution) override;
};


#endif //FRAMEWORK_COLOURINGVALIDATOR_H
