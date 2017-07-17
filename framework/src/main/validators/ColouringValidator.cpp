//
// Created by blueeyedhush on 30.05.17.
//

#include "ColouringValidator.h"
#include <cstdio>
#include <mpi.h>
#include <utils/MPIAsync.h>

bool ColouringValidator::validate(GraphPartition *g, int *partialSolution) {
	int nodeId = g->getNodeId();
	fprintf(stderr, "[VALIDATOR] Entering validator\n");

	MPI_Win partialSolutionWin;
	MPI_Win_create(partialSolution, g->getMaxLocalVertexCount()*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD,
	               &partialSolutionWin);
	MPI_Win_lock_all(0, partialSolutionWin);
	fprintf(stderr, "[VALIDATOR] Created and locked window\n");

	MPIAsync scheduler;

	fprintf(stderr, "[VALIDATOR] Starting local vertex scan\n");
	bool solutionCorrect = true;
	int processed = 0;
	g->forEachLocalVertex([g, &solutionCorrect, &partialSolutionWin, &scheduler, partialSolution, &processed](LocalVertexId v_id) {
		g->forEachNeighbour(v_id, [g,
				partialSolution,
				v_id,
				&partialSolutionWin,
				&solutionCorrect,
				&scheduler,
				&processed](GlobalVertexId neigh_id) {
			int nodeId = g->getNodeId();
			if( g->isLocalVertex(neigh_id) ) {
				/* colours for both vertices on this node */
				if(partialSolution[neigh_id.localId] == partialSolution[v_id]) {
					solutionCorrect = false;

					GlobalVertexId start;
					start.nodeId = g->getNodeId();
					start.localId = v_id;
					fprintf(stderr, "[VALIDATOR]: (%d,%d,%llu) colour %d, (%d,%d,%llu) colour %d\n", nodeId,
					        v_id, g->toNumerical(start), partialSolution[v_id], neigh_id.nodeId, neigh_id.localId,
					        g->toNumerical(neigh_id), partialSolution[neigh_id.localId]);

					processed += 1;
				}
			} else {
				/* need to query other node */
				int *colour = new int;
				MPI_Request	*rq = new MPI_Request;
				int v_colour = partialSolution[v_id];
				MPI_Rget(colour, 1, MPI_INT, neigh_id.nodeId, neigh_id.localId, 1, MPI_INT, partialSolutionWin, rq);
				scheduler.submitWaitingTask(rq, [v_colour, colour, nodeId, &solutionCorrect, &processed]() {
					if(v_colour == *colour) {
						fprintf(stderr, "[VALIDATOR]: Illegal colouring between local and remote node\n");
						solutionCorrect = false;
					}

					processed += 1;
				});
			}
		});
	});

	fprintf(stderr, "[VALIDATOR] Local vertices scanned, flushing window\n");
	MPI_Win_flush_all(partialSolutionWin);

	fprintf(stderr, "[VALIDATOR] Entering polling loop\n");
	while(processed < g->getLocalVertexCount()) {
		scheduler.pollAll();
	}
	fprintf(stderr, "[VALIDATOR] Polling done, shutting down\n");

	/*
	@todo with shutdown uncommented following error appears:
	  [0] Fatal error in PMPI_Cancel: Other MPI error, error stack:
	  [0] PMPI_Cancel(201).........: MPI_Cancel(request=0x1775120) failed
	  [0] MPIR_Cancel_impl(101)....:
	  [0] MPIR_Grequest_cancel(396): user request cancel function returned error code 738400015

	scheduler.shutdown();
	*/

	MPI_Win_unlock_all(partialSolutionWin);
	MPI_Win_free(&partialSolutionWin);

	fprintf(stderr, "ISVALID: %d\n", solutionCorrect);
	bool allProcessesHaveCorrect = false;
	MPI_Allreduce(&solutionCorrect, &allProcessesHaveCorrect, 1, MPI_CXX_BOOL, MPI_LAND, MPI_COMM_WORLD);

	return allProcessesHaveCorrect;
}
