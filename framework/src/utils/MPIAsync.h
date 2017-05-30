//
// Created by blueeyedhush on 25.05.17.
//

#ifndef FRAMEWORK_MPIASYNC_H
#define FRAMEWORK_MPIASYNC_H

#include <functional>
#include <mpi.h>
#include <vector>

class MPIAsync {
public:
	struct Callback {
		virtual void operator()() = 0;
	};

	struct MPIRequestCleaner {
		virtual void operator()(MPI_Request*) = 0;
	};
private:
	struct El {
		MPI_Request *rq;
		/* either cb or fun are set - depends on whether cb == nullptr or not */
		std::function<void(void)> fun;
		Callback *cb;
	};

public:
	MPIAsync(MPIRequestCleaner *requestCleaner, bool cleanUpCleaner = true);

	/**
	 *
	 * @param request - when request is no longer needed, requestCleander'll be called
	 * @param callback - must deallocate itself
	 */
	void submitTask(Callback *callback);
	void submitTask(std::function<void(void)> callback);
	void submitWaitingTask(MPI_Request *request, Callback *callback);
	void submitWaitingTask(MPI_Request *request, std::function<void(void)> callback);
	void pollNext(size_t x);
	void pollAll();
	void shutdown();

private:
	size_t nextToProcess;
	std::vector<El> *taskList;
	MPIRequestCleaner *cleaner;
	bool cleanUpCleaner;

	void executeCallbackAt(size_t i);
};


#endif //FRAMEWORK_MPIASYNC_H
