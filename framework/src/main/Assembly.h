//
// Created by blueeyedhush on 05.10.17.
//

#ifndef FRAMEWORK_RUNNER_H
#define FRAMEWORK_RUNNER_H

#include <mpi.h>
#include <glog/logging.h>
#include <utils/Probe.h>
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
class AlgorithmAssembly : public Assembly {
	using G = typename TGHandle::GPType;

public:
	bool algorithmSucceeded = false;
	bool validationSucceeded = false;

	virtual void run() override {
		int rank, size;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &size);

		Probe graphLoadingProbe("GraphLoading");
		Probe algorithmExecutionProbe("Algorithm");
		Probe algorithmGlobalProbe("Algorithm", true);
		Probe validationProbe("Validation");

		graphLoadingProbe.start();
		TGHandle& handle = getHandle();
		auto& graph = handle.getGraph();
		graphLoadingProbe.stop();

		if (rank == 0) algorithmGlobalProbe.start();
		algorithmExecutionProbe.start();
		TAlgorithm<G>& algorithm = getAlgorithm(handle);
		algorithmSucceeded = algorithm.run(&graph);
		algorithmExecutionProbe.stop();

		MPI_Barrier(MPI_COMM_WORLD);
		if (rank == 0) algorithmGlobalProbe.stop();

		auto solution = algorithm.getResult();

		validationProbe.start();
		TValidator<G>& validator = getValidator(handle, algorithm);
		validationSucceeded = validator.validate(&graph, solution);
		validationProbe.stop();

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
