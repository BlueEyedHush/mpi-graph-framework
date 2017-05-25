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
		Callback *cb;
	};

public:
	MPIAsync(MPIRequestCleaner *requestCleaner, bool cleanUpCleaner = true);

	/**
	 *
	 * @param request - when request is no longer needed, requestCleander'll be called
	 * @param callback - must deallocate itself
	 */
	void callWhenFinished(MPI_Request *request, Callback *callback);
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
