//
// Created by blueeyedhush on 10.05.18.
//

#ifndef FRAMEWORK_PROBE_H
#define FRAMEWORK_PROBE_H

#include <string>
#include <chrono>
#include <assert.h>
#include <glog/logging.h>

class Probe {
public:
	Probe(std::string name) : name(name), started(false) {};

	void start() {
		assert(!started);
		startTimePoint = std::chrono::system_clock::now();
		started = true;
	};

	void stop() {
		assert(started);
		auto duration = std::chrono::system_clock::now() - startTimePoint;
		auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);

		LOG(WARNING) << "[TM:" << name << ":" << durationNs.count() << "ns]";
	}

private:
	std::string name;
	bool started;
	std::chrono::system_clock::time_point startTimePoint;
};


#endif //FRAMEWORK_PROBE_H
