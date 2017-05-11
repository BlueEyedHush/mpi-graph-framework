//
// Created by blueeyedhush on 06.05.17.
//

#include <cstdio>
#include "Graph.h"

void printGraph(Graph *g) {
	for(SIZE_TYPE i = 0; i < g->getVertexCount(); i++) {
		printf("%" SCNd16 ": ", i);
		Iterator<VERTEX_ID_TYPE> *neighIt = g->getNeighbourIterator(i);
		while(neighIt->hasNext()) {
			VERTEX_ID_TYPE id = neighIt->next();
			printf("%" SCNd16 ", ", id);
		}
		delete neighIt;
		printf("\n");
	}
}