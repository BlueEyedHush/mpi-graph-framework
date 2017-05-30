//
// Created by blueeyedhush on 25.05.17.
//

#ifndef FRAMEWORK_GRAPHCOLOURINGASYNC_H
#define FRAMEWORK_GRAPHCOLOURINGASYNC_H

#include <Algorithm.h>

class GraphColouringMPAsync : public Algorithm<int*> {
public:
	GraphColouringMPAsync();
	virtual bool run(GraphPartition *g) override;
	/**
	 *
	 * @return colouring for local vertices
	 */
	virtual int *getResult() override;
	virtual ~GraphColouringMPAsync() override ;

private:
	int *finalColouring;
};


#endif //FRAMEWORK_GRAPHCOLOURINGASYNC_H
