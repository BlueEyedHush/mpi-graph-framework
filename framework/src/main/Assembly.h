//
// Created by blueeyedhush on 05.10.17.
//

#ifndef FRAMEWORK_RUNNER_H
#define FRAMEWORK_RUNNER_H

#include <mpi.h>
#include "GraphPartition.h"
#include "Algorithm.h"
#include "Validator.h"

class Assembly {
public:
	virtual void run() = 0;
	virtual ~Assembly() {};
};

/**
 *
 * This class doesn't perform any cleanup of resources that were passed it - however, you can
 * assume that as soon as run returns, they can be cleaned up
 *
 * @tparam TGHandle
 * @tparam TAlgorithm
 * @tparam TValidator
 */
template <
		class TGHandle,
		template <typename> class TAlgorithm,
		template <typename> class TValidator
		>
class AlgorithmAssembly : Assembly {
	using G = typename TGHandle::GPType;

public:
	bool algorithmSucceeded = false;
	bool validationSucceeded = false;

	virtual void run() override {
		TGHandle& handle = getHandle();
		auto& graph = handle.getGraph();

		TAlgorithm<G>& algorithm = getAlgorithm(handle);
		algorithmSucceeded = algorithm.run(&graph);

		MPI_Barrier(MPI_COMM_WORLD);

		auto solution = algorithm.getResult();

		TValidator<G>& validator = getValidator(handle, algorithm);
		validationSucceeded = validator.validate(&graph, solution);

		if (!algorithmSucceeded) {
			LOG(ERROR) << "Error occured while executing algorithm";
		} else {
			LOG(INFO) << "Algorithm terminated successfully";
		}

		if(!validationSucceeded) {
			LOG(ERROR) << "Validation failure";
		} else {
			LOG(INFO) << "Validation success";
		}

		handle.releaseGraph();
	}

protected:
	virtual TGHandle& getHandle() = 0;
	virtual TAlgorithm<G>& getAlgorithm(TGHandle&) = 0;
	virtual TValidator<G>& getValidator(TGHandle&, TAlgorithm<G>&) = 0;
};

#endif //FRAMEWORK_RUNNER_H
