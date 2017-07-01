//
// Created by blueeyedhush on 01.07.17.
//

#ifndef FRAMEWORK_INDEXPARTITIONER_H
#define FRAMEWORK_INDEXPARTITIONER_H

#include <utility>

namespace IndexPartitioner {

int get_partition_from_index(int element_count, int partition_count, int index);
/**
 * Each node gets vertex_no/world_size vertices. The excess (k = vertex_no % world_rank) is distributed between
 * first k nodes
 *
 * @param partition_no - rank of the requesting node
 * @return std::pair where first is index of first vertex and second is index of one vertex after last
 */
std::pair <int, int> get_range_for_partition(int element_count, int partition_count, int partition_no);

}

#endif //FRAMEWORK_INDEXPARTITIONER_H
