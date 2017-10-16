//
// Created by blueeyedhush on 14.07.17.
//

#ifndef FRAMEWORK_BFSVALIDATOR_H
#define FRAMEWORK_BFSVALIDATOR_H


#include <utility>
#include <Validator.h>

class BfsValidator : public Validator<std::pair<GlobalVertexId*, int*>*> {
public:
	BfsValidator(const GlobalVertexId& _root) : root(_root) {};
	// @ToDo - (types) path length should be parametrizable + registering type with MPI
	virtual bool validate(GraphPartition *g, std::pair<GlobalVertexId*, int*> *partialSolution) override;
private:
	const GlobalVertexId& root;
};


#endif //FRAMEWORK_BFSVALIDATOR_H
