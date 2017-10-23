//
// Created by blueeyedhush on 06.05.17.
//

#ifndef FRAMEWORK_GRAPHCOLOURING_H
#define FRAMEWORK_GRAPHCOLOURING_H

#include <algorithms/Colouring.h>

class GraphColouringMP : public Algorithm<VertexColour*> {
public:
	GraphColouringMP();
	virtual bool run(GraphPartition *g) override;
	virtual VertexColour *getResult() override;
	virtual ~GraphColouringMP() override;

private:
	int* finalColouring;
};

#endif //FRAMEWORK_GRAPHCOLOURING_H
