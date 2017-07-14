//
// Created by blueeyedhush on 03.07.17.
//

#ifndef FRAMEWORK_TESTUTILS_H
#define FRAMEWORK_TESTUTILS_H

#include <string>
#include <vector>
#include <GraphPartition.h>

std::vector<std::pair<GlobalVertexId, int>> bspSolutionFromFile(std::string path);

int* loadPartialIntSolution(std::string solutionFilePath, int partitionCount, int partitionId);
std::pair<GlobalVertexId, int>* loadBspSolutionFromFile(std::string path, int partitionCount, int partitionId);

#endif //FRAMEWORK_TESTUTILS_H
