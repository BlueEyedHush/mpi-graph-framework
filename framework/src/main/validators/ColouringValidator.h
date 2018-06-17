//
// Created by blueeyedhush on 30.05.17.
//

#ifndef FRAMEWORK_COLOURINGVALIDATOR_H
#define FRAMEWORK_COLOURINGVALIDATOR_H

#include <mpi.h>
#include <glog/logging.h>
#include <utils/MPIAsync.h>
#include <Validator.h>
#include <algorithms/Colouring.h>

#define CV_LOCAL_SHORTCIRCUIT 0

template <typename TGraphPartition>
class ColouringValidator : public Validator<TGraphPartition, VertexColour *> {
private:
	IMPORT_ALIASES(TGraphPartition)

public: /* @todo: finish rewriting validator */
	bool validate(TGraphPartition *g, VertexColour *partialSolution) {
		int nodeId;
		MPI_Comm_rank(MPI_COMM_WORLD, &nodeId);
		LOG(INFO) << "Entering validator";

		MPI_Win partialSolutionWin;
		MPI_Win_create(partialSolution, g->masterVerticesMaxCount()*sizeof(VertexColour), sizeof(VertexColour),
		               MPI_INFO_NULL, MPI_COMM_WORLD, &partialSolutionWin);
		MPI_Win_lock_all(0, partialSolutionWin);
		LOG(INFO) << "Created and locked window";

		MPIAsync scheduler;

		LOG(INFO) << "Starting local vertex scan";
		bool solutionCorrect = true;
		int processed = 0;
		size_t requestsMade = 0;
		/* @todo: correct indentation & wrapping - tweak CLion rules */
		g->foreachMasterVertex([&, g, partialSolution, nodeId](const LocalId v_id) {
			g->foreachNeighbouringVertex(v_id, [&, g, nodeId, partialSolution, v_id](const GlobalId neigh_id) {
				auto neighLocalId = g->toLocalId(neigh_id);
				#if CV_LOCAL_SHORTCIRCUIT == 1
				if(g->toMasterNodeId(neigh_id) == nodeId) {
					/* colours for both vertices on this node */
					if(partialSolution[neighLocalId] == partialSolution[v_id]) {
						solutionCorrect = false;

						LOG(INFO) << "Failure: "
						          << g->idToString(v_id) << "(" << g->toNumeric(v_id) << ") "
						          << "colour: " << partialSolution[v_id] << ", "
						          << g->idToString(neigh_id) << "(" << g->toNumeric(neigh_id) << ") "
						          << "colour: " << partialSolution[neighLocalId];

					}

					processed += 1;
				} else {
				#endif
					/* need to query other node */
					int *colour = new int;
					MPI_Request	*rq = new MPI_Request;
					int v_colour = partialSolution[v_id];
					MPI_Rget(colour, 1, MPI_INT, g->toMasterNodeId(neigh_id), neighLocalId, 1, MPI_INT, partialSolutionWin, rq);
					scheduler.submitWaitingTask(rq, [v_colour, colour, nodeId, &solutionCorrect, &processed]() {
						if(v_colour == *colour) {
							LOG(ERROR) << "Illegal colouring between local and remote node";
							solutionCorrect = false;
						}

						processed += 1;
					});
				#if CV_LOCAL_SHORTCIRCUIT == 1
				}
				#endif

				requestsMade += 1;

				return ITER_PROGRESS::CONTINUE;
			});

			if (requestsMade >= 1000) {
				auto s_before = scheduler.getQueueSize();
				scheduler.pollAll();
				requestsMade = 0;
				LOG(INFO) << "Queue flush, went from " << s_before << " to " << scheduler.getQueueSize();
			}

			return ITER_PROGRESS::CONTINUE;
		});

		LOG(INFO) << "Local vertices scanned, flushing window";
		MPI_Win_flush_all(partialSolutionWin);

		LOG(INFO) << "Entering polling loop";
		while(processed < g->masterVerticesCount()) {
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
};


#endif //FRAMEWORK_COLOURINGVALIDATOR_H
