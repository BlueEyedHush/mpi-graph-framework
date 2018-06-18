//
// Created by blueeyedhush on 12.05.17.
//

#ifndef FRAMEWORK_BUFFERPOOL_H
#define FRAMEWORK_BUFFERPOOL_H

#include <functional>
#include <list>
#include <iterator>

template <class T>
class AutoFreeingBuffer {
	std::list<T*> freeBuffers;
	std::list<T*> allocatedBuffers;

	std::function<bool (T *)> softFreerer;
	size_t softThreshold;
	std::function<bool (T *)> hardFreerer;
	std::function<void ()> oneOffHardFreerer;
	size_t hardThreshold;

public:
	/**
	 *
	 * @param softSpaceConsumptionLimit
	 * @param freerer - callback should returns true if buffer can be considered free or false otherwise
	 */
	AutoFreeingBuffer(size_t softThreshold,
	                  size_t hardThreshold,
	                  std::function<bool(T *)> softFreerer,
	                  std::function<void(T *)> hardFreerer,
	                  std::function<void()> oneOffHardFreerer)
			: softThreshold(softThreshold),
			  hardThreshold(hardThreshold),
			  softFreerer(softFreerer),
			  hardFreerer([=](T* t) {hardFreerer(t); return true;}),
			  oneOffHardFreerer(oneOffHardFreerer)
	{}

	/**
	 * Always returns free buffer, even if it has to be allocated first
	 * @return
	 */
	T* get() {
		T *b = nullptr;
		if (freeBuffers.empty()) {
			b = new T();
		} else {
			b = freeBuffers.front();
			freeBuffers.pop_front();
		}
		allocatedBuffers.push_front(b);
		return b;
	}

	void tryFree() {
		auto abs = allocatedBuffers.size();
		if ((abs > hardThreshold)) {
			auto it = allocatedBuffers.begin();
			std::advance(it, abs - softThreshold);

			oneOffHardFreerer();
			doIter(it, hardFreerer);
		} else if (abs > softThreshold) {
			doIter(allocatedBuffers.begin(), softFreerer);
		}
	}

	void wait(bool hard = false) {
		if (hard) doIter(allocatedBuffers.begin(), hardFreerer);
		else {
			oneOffHardFreerer();
			doIter(allocatedBuffers.begin(), softFreerer);
		}
	}

private:
	void doIter(typename std::list<T*>::iterator it, std::function<bool(T *)> freerer) {
		while(it != allocatedBuffers.end()) {
			if (freerer(*it)) {
				/* buffer has been freed */
				if (freeBuffers.size() > softThreshold)
					delete *it;
				else
					freeBuffers.push_front(*it);

				it = allocatedBuffers.erase(it);
			} else {
				it++;
			}
		}
	}

};



template <class T> class BufferPool {
private:
	std::list<T*> freeBuffers;
	std::list<T*> allocatedBuffers;

public:
	BufferPool(size_t initialSize = 0) {
		addEmptyBuffers(initialSize);
	}

	~BufferPool() {
		for(auto b: freeBuffers) {
			delete b;
		}

		if(!allocatedBuffers.empty()) {
			fprintf(stderr, "WARN: some buffers in BufferPool remain not freed!");
			for(auto b: allocatedBuffers) {
				delete b;
			}
		}
	}

	/**
	 * Iterates over all free.
	 * To singal that buffer is now used, return true.
	 * Otherwise return false
	 * @param f
	 */
	void foreachFree(const std::function<bool(T *)> &f) {
		iterate(freeBuffers, allocatedBuffers, f);
	}

	/**
	 * Iterates over all used buffers.
	 * To singal that buffer has been freed, return true from f.
	 * If you're still using the buffer, return false
	 * @param f
	 */
	void foreachUsed(const std::function<bool(T *)> &f) {
		iterate(allocatedBuffers, freeBuffers, f);
	}

private:
	static void iterate(std::list<T*> &first,
	                    std::list<T*> &second,
	                    const std::function<bool(T*)> &f) {
		for (auto it = first.begin(); it != first.end();) {
			if (f(*it)) {
				second.push_front(*it);
				it = first.erase(it);
			} else {
				it++;
			}
		}
	}

	void addEmptyBuffers(size_t count) {
		for(int i = 0; i < count; i++) {
			freeBuffers.push_front(new T);
		}
	}
};


#endif //FRAMEWORK_BUFFERPOOL_H
