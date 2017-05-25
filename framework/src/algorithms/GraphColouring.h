//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPHCOLOURING_H
#define FRAMEWORK_GRAPHCOLOURING_H

#include <Algorithm.h>

class GraphColouringMP : public Algorithm {
	virtual bool run(GraphPartition *g) override;
};

class GraphColouringMPAsync : public Algorithm {
	virtual bool run(GraphPartition *g) override;
};

class GraphColouringRMA : public Algorithm {
	virtual bool run(GraphPartition *g) override;
};

#endif //FRAMEWORK_GRAPHCOLOURING_H
