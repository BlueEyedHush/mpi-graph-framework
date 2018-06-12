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
	Probe(std::string name, bool global = false) : name(name), global(global), started(false) {};

	void start() {
		assert(!started);
		startTimePoint = std::chrono::system_clock::now();
		started = true;
	};

	void stop() {
		assert(started);
		auto duration = std::chrono::system_clock::now() - startTimePoint;
		auto durationNs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration);
		std::string probeType = global ? "TG" : "TL";

		/* time is reported in nanoseconds */
		LOG(WARNING) << "[P:" << probeType << ":" << name << ":" << durationNs.count() << "]";
	}

private:
	std::string name;
	bool started;
	bool global;
	std::chrono::system_clock::time_point startTimePoint;
};


class MemProbe {
public:
	static void reportFraction(std::string name, size_t x, size_t from) {
		LOG(WARNING) << "[P:MF:" << name << ":" << x << "/" << from << "]";
	}
};

#endif //FRAMEWORK_PROBE_H
