//
// Created by blueeyedhush on 24.11.17.
//

#ifndef FRAMEWORK_NONCOPYABLE_H
#define FRAMEWORK_NONCOPYABLE_H

class NonCopyable {
protected:
	NonCopyable() {}

	~NonCopyable() {} /// Protected non-virtual destructor
private:
	NonCopyable(const NonCopyable &) = delete;

	NonCopyable &operator=(const NonCopyable &) = delete;
};

#endif //FRAMEWORK_NONCOPYABLE_H
