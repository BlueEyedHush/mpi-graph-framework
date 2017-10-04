//
// Created by blueeyedhush on 04.10.17.
//

#ifndef FRAMEWORK_VARIABLELENGTHBUFFERMANAGER_H
#define FRAMEWORK_VARIABLELENGTHBUFFERMANAGER_H


#include <cstddef>

template <class T> class VariableLengthBufferManager {
public:
	VariableLengthBufferManager() : buffer(nullptr), size(0) {}

	~VariableLengthBufferManager() {
		if(buffer != nullptr) delete[] buffer;
	}

	/**
	 *
	 * @param expectedCapacity
	 * @return pointer remains valid until next call to getBuffer
	 */
	T* getBuffer(size_t expectedCapacity) {
		if(buffer != nullptr) delete[] buffer;
		size = expectedCapacity;
		buffer = new T[expectedCapacity];
		return buffer;
	}

	VariableLengthBufferManager(const VariableLengthBufferManager&) = delete;
	VariableLengthBufferManager& operator=(const VariableLengthBufferManager&) = delete;

private:
	T* buffer;
	size_t size;
};


#endif //FRAMEWORK_VARIABLELENGTHBUFFERMANAGER_H
