//
// Created by blueeyedhush on 30.05.17.
//

#include <set>
#include "ColouringValidator.h"

bool ColouringValidator::validate(GraphPartition *g, int *partialSolution) {
	int N = g->getLocalVertexCount();

	bool solutionCorrect = true;
	g->forEachLocalVertex([&g, &solutionCorrect, partialSolution](LocalVertexId v_id) {
		int v_id_colour = partialSolution[v_id];
		g->forEachNeighbour(v_id, [partialSolution, &solutionCorrect, v_id_colour](GlobalVertexId neigh_id) {
			solutionCorrect = partialSolution[neigh_id.localId] != v_id_colour;
		});
	});

	return solutionCorrect;
}
