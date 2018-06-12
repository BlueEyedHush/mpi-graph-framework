//
// Created by blueeyedhush on 05.10.17.
//

#ifndef FRAMEWORK_RUNNER_H
#define FRAMEWORK_RUNNER_H

#include <mpi.h>
#include <glog/logging.h>
#include <utils/Probe.h>
#include <utils/Config.h>
#include "GraphPartition.h"
#include "Algorithm.h"
#include "Validator.h"

class Executor;

class Assembly {
public:
	void setExecutor(Executor *e) {
		if (parentExecutor != nullptr)
			throw std::runtime_error("Assembly cannot be assigned to different executor");

		parentExecutor = e;
	}

	void run(ConfigMap cmap) {
		if (parentExecutor == nullptr)
			throw std::runtime_error("Executor must be set before running assembly");

		doRun(cmap);
	};

	virtual ~Assembly() {};

protected:
	virtual void doRun(ConfigMap) = 0;

protected:
	Executor* parentExecutor = nullptr;
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

	void doRun(ConfigMap config) override {
		LOG(INFO) << "Started executing AlgorithmAssembly";

		int rank, size;
		MPI_Comm_rank(MPI_COMM_WORLD, &rank);
		MPI_Comm_size(MPI_COMM_WORLD, &size);

		Probe graphLoadingProbe("GraphLoading");
		Probe graphGlobalProbe("GraphLoading", true);
		Probe algorithmExecutionProbe("Algorithm");
		Probe algorithmGlobalProbe("Algorithm", true);
		Probe validationProbe("Validation");

		LOG(INFO) << "About to start graph loading";

		MPI_Barrier(MPI_COMM_WORLD);
		if (rank == 0) graphGlobalProbe.start();
		graphLoadingProbe.start();
		TGHandle& handle = getHandle();
		auto& graph = handle.getGraph();
		graphLoadingProbe.stop();

		LOG(INFO) << "Graph has been loaded";

	    MPI_Barrier(MPI_COMM_WORLD);
		if (rank == 0) {
			graphGlobalProbe.stop();
			algorithmGlobalProbe.start();
		}

		AAuxiliaryParams aaParams;
		aaParams.config = config;

		algorithmExecutionProbe.start();
		TAlgorithm<G>& algorithm = getAlgorithm(handle);
		algorithmSucceeded = algorithm.run(&graph, aaParams);
		algorithmExecutionProbe.stop();

		LOG(INFO) << "Algorithm has been executed";

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
