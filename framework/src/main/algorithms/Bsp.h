//
// Created by blueeyedhush on 18.07.17.
//

#ifndef FRAMEWORK_BSP_H
#define FRAMEWORK_BSP_H

#include <Algorithm.h>
#include <utility>

class Bsp_Mp_FixedMessageSize_1D_2CommRounds : public Algorithm<std::pair<GlobalVertexId*, int*>*> {
public:
	Bsp_Mp_FixedMessageSize_1D_2CommRounds();
	virtual bool run(GraphPartition *g) override;
	virtual std::pair<GlobalVertexId*, int*> *getResult() override;
	virtual ~Bsp_Mp_FixedMessageSize_1D_2CommRounds() override;

private:
	std::pair<GlobalVertexId*, int*> result;

	GlobalVertexId& getPredecessor(int vid) {
		return result.first[vid];
	}

	int& getDistance(int vid) {
		return result.second[vid];
	}
};


#endif //FRAMEWORK_BSP_H
