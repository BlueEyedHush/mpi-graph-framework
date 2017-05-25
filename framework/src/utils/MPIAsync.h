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
	MPIAsync();

	/**
	 *
	 * @param request - takes ownership of the memory and'll clean it up
	 */
	void callWhenFinished(MPI_Request *request, const std::function<void(void)> callback);
	void pollNext(int x);
	void pollAll();
	void shutdown();

private:
	int nextToProcess;
	std::vector<MPI_Request*> *requests;
	std::vector<std::function<void(void)>> *callbacks;

	void executeCallbackAt(MPI_Request *rq, int i);
};


#endif //FRAMEWORK_MPIASYNC_H
