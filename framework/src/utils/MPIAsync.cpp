//
// Created by blueeyedhush on 25.05.17.
//

#include "MPIAsync.h"

MPIAsync::MPIAsync() {
	nextToProcess = 0;
	taskList = new std::vector<El>();
}

void MPIAsync::callWhenFinished(MPI_Request *request, Callback *callback) {
	El el;
	el.cb = callback;
	el.rq = request;
	taskList->push_back(el);
}

void MPIAsync::pollNext(size_t x) {
	for(size_t i = 0; i < x; i++) {
		if (nextToProcess >= taskList->size()) {
			nextToProcess = 0;
		}

		El el = taskList->at(nextToProcess);

		if (el.rq == nullptr) {
			/* no need to wait, can execute immediatelly */
			executeCallbackAt(nextToProcess);
		} else {
			int result = 0;
			MPI_Test(el.rq, &result, MPI_STATUS_IGNORE);
			if (result != 0) {
				MPI_Wait(el.rq, MPI_STATUS_IGNORE);

				executeCallbackAt(nextToProcess);
			}
		}

		nextToProcess++;
	}
}

void MPIAsync::pollAll() {
	if (nextToProcess >= taskList->size()) {
		nextToProcess = 0;
	}
	size_t toPoll = taskList->size() - nextToProcess;
	pollNext(toPoll);
}

void MPIAsync::shutdown() {
	for(int i = 0; i < taskList->size(); i++) {
		El el = taskList->at(i);
		if (el.rq != nullptr) {
			MPI_Cancel(el.rq);
			delete el.rq;
		}
	}
}

void MPIAsync::executeCallbackAt(size_t i) {
	El el = taskList->at(i);
	el.cb->operator()();

	if (taskList->size() >= 2) {
		size_t lastIdx = taskList->size() - 1;
		taskList->at(i) = taskList->at(lastIdx);
		taskList->pop_back();
	} else {
		taskList->clear();
	}

	if (el.rq != nullptr) {
		delete el.rq;
	}
}
