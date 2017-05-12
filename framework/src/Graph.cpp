//
// Created by blueeyedhush on 06.05.17.
//

#include <cstdio>
#include "Graph.h"

void printGraph(Graph *g) {
	for(int i = 0; i < g->getVertexCount(); i++) {
		printf("%" SCNd16 ": ", i);
		g->forEachNeighbour(i, [&](int id) {
			printf("%" SCNd16 ", ", id);
		});
		printf("\n");
	}
}