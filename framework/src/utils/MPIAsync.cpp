//
// Created by blueeyedhush on 25.05.17.
//

#include "MPIAsync.h"

MPIAsync::MPIAsync() {
	nextToProcess = 0;
	requests = new std::vector<MPI_Request*>();
	callbacks = new std::vector<std::function<void(void)>>();
}

void MPIAsync::callWhenFinished(MPI_Request *request, const std::function<void(void)> callback) {
	requests->push_back(request);
	callbacks->push_back(callback);
}

void MPIAsync::pollNext(int x) {
	for(int i = 0; i < x; i++) {
		MPI_Request *rq = requests->at(i);

		if (rq == nullptr) {
			/* no need to wait, can execute immediatelly */
			executeCallbackAt(rq, i);
		} else {
			int result = 0;
			MPI_Test(rq, &result, MPI_STATUS_IGNORE);
			if (result != 0) {
				MPI_Wait(rq, MPI_STATUS_IGNORE);

				executeCallbackAt(rq, i);
			}
		}

		if (nextToProcess >= requests->size()) {
			nextToProcess = 0;
		}
	}
}

void MPIAsync::pollAll() {
	int toPoll = requests->size() - nextToProcess;
	pollNext(toPoll);
}

void MPIAsync::shutdown() {
	for(int i = 0; i < requests->size(); i++) {
		MPI_Request *rq = requests->at(i);
		if (rq != nullptr) {
			MPI_Cancel(rq);
		}
	}
}

void MPIAsync::executeCallbackAt(MPI_Request *rq, int i) {
	auto cb = callbacks->at(i);
	cb();

	if (requests->size() >= 2) {
		size_t lastIdx = requests->size() - 1;

		requests->at(i) = requests->at(lastIdx);
		callbacks->at(i) = callbacks->at(lastIdx);

		requests->pop_back();
		callbacks->pop_back();
	} else {
		requests->clear();
		callbacks->clear();
	}

	if (rq != nullptr) {
		delete rq;
	}
}
