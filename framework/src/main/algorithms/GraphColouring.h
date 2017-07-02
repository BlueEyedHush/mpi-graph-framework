//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPHCOLOURING_H
#define FRAMEWORK_GRAPHCOLOURING_H

#include <Algorithm.h>

class GraphColouringMP : public Algorithm<int*> {
	virtual bool run(GraphPartition *g) override;
	virtual int *getResult() override;
	virtual ~GraphColouringMP() override;
};

#endif //FRAMEWORK_GRAPHCOLOURING_H
