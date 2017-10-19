//
// Created by blueeyedhush on 05.10.17.
//

#ifndef FRAMEWORK_RUNNER_H
#define FRAMEWORK_RUNNER_H

#include <mpi.h>
#include "GraphPartition.h"
#include "Algorithm.h"
#include "Validator.h"

struct AlgorithmExecutionResult {
	bool algorithmStatus = false;
	bool validatorStatus = false;
};

template<class TGraphPartition, typename TResult>
AlgorithmExecutionResult runAndCheck(TGraphPartition *graph,
                                     std::function<Algorithm<TResult, TGraphPartition> *(void)> algorithmProvider,
                                     std::function<Validator<TGraphPartition, TResult> *(void)> validatorProvider)
{
	auto *algorithm = algorithmProvider();
	auto *validator = validatorProvider();
	AlgorithmExecutionResult r;

	r.algorithmStatus = algorithm->run(graph);

	MPI_Barrier(MPI_COMM_WORLD);

	TResult solution = algorithm->getResult();
	r.validatorStatus = validator->validate(graph, solution);

	return r;
}

#endif //FRAMEWORK_RUNNER_H
