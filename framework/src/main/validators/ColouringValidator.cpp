//
// Created by blueeyedhush on 30.05.17.
//

#include "ColouringValidator.h"
#include <cstdio>
#include <mpi.h>
#include <utils/MPIAsync.h>

bool ColouringValidator::validate(GraphPartition *g, int *partialSolution) {
	int nodeId = g->getNodeId();
	fprintf(stderr, "[%d][VALIDATOR] Entering validator\n", nodeId);

	MPI_Win partialSolutionWin;
	MPI_Win_create(partialSolution, g->getMaxLocalVertexCount()*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD,
	               &partialSolutionWin);
	MPI_Win_lock_all(0, partialSolutionWin);
	fprintf(stderr, "[%d][VALIDATOR] Created and locked window\n", nodeId);

	MPIAsync scheduler;

	fprintf(stderr, "[%d][VALIDATOR] Starting local vertex scan\n", nodeId);
	bool solutionCorrect = true;
	g->forEachLocalVertex([g, &solutionCorrect, &partialSolutionWin, &scheduler, partialSolution](LocalVertexId v_id) {
		g->forEachNeighbour(v_id, [g,
				partialSolution,
				v_id,
				&partialSolutionWin,
				&solutionCorrect,
				&scheduler](GlobalVertexId neigh_id) {
			int nodeId = g->getNodeId();
			if( g->isLocalVertex(neigh_id) ) {
				/* colours for both vertices on this node */
				if(partialSolution[neigh_id.localId] == partialSolution[v_id]) {
					solutionCorrect = false;

					GlobalVertexId start;
					start.nodeId = g->getNodeId();
					start.localId = v_id;
					fprintf(stderr, "[%d][VALIDATOR]: (%d,%d,%llu) colour %d, (%d,%d,%llu) colour %d\n", nodeId, nodeId,
					        v_id, g->toNumerical(start), partialSolution[v_id], neigh_id.nodeId, neigh_id.localId,
					        g->toNumerical(neigh_id), partialSolution[neigh_id.localId]);
				}
			} else {
				/* need to query other node */
				int *colour = new int;
				MPI_Request	*rq = new MPI_Request;
				int v_colour = partialSolution[v_id];
				MPI_Rget(colour, 1, MPI_INT, neigh_id.nodeId, neigh_id.localId, 1, MPI_INT, partialSolutionWin, rq);
				scheduler.submitWaitingTask(rq, [v_colour, colour, nodeId, &solutionCorrect]() {
					fprintf(stderr, "[%d][VALIDATOR]: Entering callback\n", nodeId);
					if(v_colour == *colour) {
						fprintf(stderr, "[%d][VALIDATOR]: Illegal colouring between local and remote node\n", nodeId);
					}
				});
			}
		});
	});

	fprintf(stderr, "[%d][VALIDATOR] Local vertices scanned, flushing window\n", nodeId);
	MPI_Win_flush_all(partialSolutionWin);
	/* as soon as we find any problem with colouring, we can stop and report failure */
	fprintf(stderr, "[%d][VALIDATOR] Entering polling loop\n", nodeId);
	bool anythingLeft = true;
	while(solutionCorrect && anythingLeft) {
		anythingLeft = scheduler.pollAll();
	}
	fprintf(stderr, "[%d][VALIDATOR] Polling done, shutting down\n", nodeId);

	scheduler.shutdown();
	MPI_Win_unlock_all(partialSolutionWin);
	MPI_Win_free(&partialSolutionWin);

	bool allProcessesHaveCorrect = false;
	MPI_Allreduce(&solutionCorrect, &allProcessesHaveCorrect, 1, MPI_CXX_BOOL, MPI_LAND, MPI_COMM_WORLD);

	return allProcessesHaveCorrect;
}
