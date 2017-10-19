//
// Created by blueeyedhush on 17.10.17.
//

#ifndef FRAMEWORK_PREREQUISITES_H
#define FRAMEWORK_PREREQUISITES_H

typedef unsigned long long OriginalVertexId;

typedef int NodeId;
#define NODE_ID_MPI_TYPE MPI_INT

typedef int GraphDist;
#define GRAPH_DIST_MPI_TYPE MPI_INT

/* types used by default across framework (they are only passed to parameters, so ofc other can be used) */
// @ToDo(after interface change): or maybe nomore?
#define LOCAL_VERTEX_ID_MPI_TYPE MPI_UNSIGNED_LONG_LONG
typedef unsigned long long LocalVertexId;
#define NUMID_MPI_TYPE MPI_UNSIGNED_LONG_LONG
typedef unsigned long long NumericIdRepr;

#endif //FRAMEWORK_PREREQUISITES_H
