//
// Created by blueeyedhush on 14.07.17.
//

#ifndef FRAMEWORK_UNIQUEIDGENERATOR_H
#define FRAMEWORK_UNIQUEIDGENERATOR_H

class UniqueIdGenerator {
public:
	typedef unsigned int Id;
	Id next();
private:
	Id nextId = 0;
};


#endif //FRAMEWORK_UNIQUEIDGENERATOR_H
