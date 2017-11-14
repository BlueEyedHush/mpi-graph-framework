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

template<class TGraphPartition, typename TAlgorithm, typename TValidator>
AlgorithmExecutionResult runAndCheck(TGraphPartition* graph, TAlgorithm& algorithm, TValidator& validator) {
	AlgorithmExecutionResult r;

	r.algorithmStatus = algorithm.run(graph);

	MPI_Barrier(MPI_COMM_WORLD);

	auto solution = algorithm.getResult();
	r.validatorStatus = validator.validate(graph, solution);

	return r;
}

class Assembly {
	virtual void run() = 0;
	virtual ~Assembly() {};
};

template <typename TGHandle, typename TAlgorithm, typename TValidator>
class AlgorithmAssembly : Assembly {
public:
	AlgorithmAssembly(std::function<TGHandle&()> ghProvider,
	                  std::function<TAlgorithm&(TGHandle&)> algorithmProvider,
	                  std::function<TValidator&(TAlgorithm&)> validatorProvider)
			: ghp(ghProvider), ap(algorithmProvider), vp(validatorProvider) {}

	virtual void run() override {
		TGHandle& handle = ghp();
		auto graph = handle->getGraph();

		TAlgorithm& algorithm = ap(handle);
		bool algorithmStatus = algorithm.run(graph);

		MPI_Barrier(MPI_COMM_WORLD);

		auto solution = algorithm.getResult();

		TValidator& validator = vp(algorithm);
		bool validationStatus = validator.validate(graph, solution);

		if (!algorithmStatus) {
			LOG(ERROR) << "Error occured while executing algorithm";
		} else {
			LOG(INFO) << "Algorithm terminated successfully";
		}

		if(!validationStatus) {
			LOG(ERROR) << "Validation failure";
		} else {
			LOG(INFO) << "Validation success";
		}

		handle->releaseGraph();
	}

private:
	std::function<TGHandle&()> ghp;
	std::function<TAlgorithm&(TGHandle&)> ap;
	std::function<TValidator&(TAlgorithm&)> vp;
};

#endif //FRAMEWORK_RUNNER_H
