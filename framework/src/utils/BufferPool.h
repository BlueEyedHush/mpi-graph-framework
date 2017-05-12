//
// Created by blueeyedhush on 12.05.17.
//

#ifndef FRAMEWORK_BUFFERPOOL_H
#define FRAMEWORK_BUFFERPOOL_H

#include <functional>
#include <list>

template <class T> class BufferPool {
private:
	std::list<T*> freeBuffers;
	std::list<T*> allocatedBuffers;

public:
	BufferPool(int initialSize = 0) {
		for(int i = 0; i < initialSize; i++) {
			freeBuffers.push_front(new T);
		}
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
	 * Always returns free buffer, even if it has to be allocated first
	 * @return
	 */
	T *getNew() {
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
};


#endif //FRAMEWORK_BUFFERPOOL_H
