//
// Created by blueeyedhush on 30.05.17.
//

#include "ColouringValidator.h"
#include <mpi.h>
#include <glog/logging.h>
#include <utils/MPIAsync.h>

bool ColouringValidator::validate(GraphPartition *g, int *partialSolution) {
	int nodeId = g->getNodeId();
	LOG(INFO) << "Entering validator";

	MPI_Win partialSolutionWin;
	MPI_Win_create(partialSolution, g->getMaxLocalVertexCount()*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD,
	               &partialSolutionWin);
	MPI_Win_lock_all(0, partialSolutionWin);
	LOG(INFO) << "Created and locked window";

	MPIAsync scheduler;

	LOG(INFO) << "Starting local vertex scan";
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

					LOG(INFO) << "(" << nodeId << "," << v_id << "," << g->toNumerical(start) << ") " <<
                              "colour " << partialSolution[v_id] << ", " <<
                              "(" << neigh_id.nodeId << "," << neigh_id.localId << "," <<  g->toNumerical(neigh_id) << ") "
					          << "colour " << partialSolution[neigh_id.localId];

				}

				processed += 1;
			} else {
				/* need to query other node */
				int *colour = new int;
				MPI_Request	*rq = new MPI_Request;
				int v_colour = partialSolution[v_id];
				MPI_Rget(colour, 1, MPI_INT, neigh_id.nodeId, neigh_id.localId, 1, MPI_INT, partialSolutionWin, rq);
				scheduler.submitWaitingTask(rq, [v_colour, colour, nodeId, &solutionCorrect, &processed]() {
					if(v_colour == *colour) {
						LOG(ERROR) << "Illegal colouring between local and remote node";
						solutionCorrect = false;
					}

					processed += 1;
				});
			}
		});
	});

	LOG(INFO) << "Local vertices scanned, flushing window";
	MPI_Win_flush_all(partialSolutionWin);

	LOG(INFO) << "Entering polling loop";
	while(processed < g->getLocalVertexCount()) {
		scheduler.pollAll();
	}
	LOG(INFO) << "Polling done, shutting down";

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

	bool allProcessesHaveCorrect = false;
	MPI_Allreduce(&solutionCorrect, &allProcessesHaveCorrect, 1, MPI_CXX_BOOL, MPI_LAND, MPI_COMM_WORLD);

	return allProcessesHaveCorrect;
}
