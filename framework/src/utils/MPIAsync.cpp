//
// Created by blueeyedhush on 25.05.17.
//

#include "MPIAsync.h"

MPIAsync::MPIAsync(MPIRequestCleaner *requestCleaner, bool cleanUpCleaner) {
	nextToProcess = 0;
	taskList = new std::vector<El>();
	cleaner = requestCleaner;
	this->cleanUpCleaner = cleanUpCleaner;
}

void MPIAsync::submitTask(MPIAsync::Callback *callback) {
	submitWaitingTask(nullptr, callback);
}

void MPIAsync::submitWaitingTask(MPI_Request *request, Callback *callback) {
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
			(*cleaner)(el.rq);
		}
	}

	if (cleanUpCleaner) {
		delete cleaner;
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
		(*cleaner)(el.rq);
	}
}

void MPIAsync::submitTask(std::function<void(void)> callback) {
	submitWaitingTask(nullptr, callback);
}

void MPIAsync::submitWaitingTask(MPI_Request *request, std::function<void(void)> callback) {
	El el;
	el.rq = request;
	el.cb = nullptr;
	el.fun = callback;
	taskList->push_back(el);
}
