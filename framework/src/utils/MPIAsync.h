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
private:
	struct El {
		MPI_Request *rq;
		Callback *cb;
	};

public:
	MPIAsync();

	/**
	 *
	 * @param request - takes ownership of the memory and'll clean it up
	 */
	void callWhenFinished(MPI_Request *request, Callback *callback);
	void pollNext(size_t x);
	void pollAll();
	void shutdown();

private:
	size_t nextToProcess;
	std::vector<El> *taskList;

	void executeCallbackAt(size_t i);
};


#endif //FRAMEWORK_MPIASYNC_H
