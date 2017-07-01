//
// Created by blueeyedhush on 01.07.17.
//

#include "IndexPartitioner.h"
#include <algorithm>

namespace IndexPartitioner {

int get_partition_from_index(int element_count, int partition_count, int index) {
	int base_width = element_count / partition_count;
	int excess = element_count % partition_count;
	int prognosed_partition = index / base_width;
	/* above would be target node if we didn't decided to partition excess the way we did
	 * excess must be smaller than bucket width, so our vertex won't go much further than node_id back */
	int prognosed_start = base_width * prognosed_partition + std::min(prognosed_partition, excess);

	int actual_partition = (index >= prognosed_start) ? prognosed_partition : prognosed_partition - 1;

	return actual_partition;
}

std::pair<int, int> get_range_for_partition(int element_count, int partition_count, int partition_no) {
	int base_width = element_count / partition_count;
	int excess = element_count % partition_count;

	int count = base_width + std::max(0, std::min(1, excess - partition_no));
	int start = base_width * partition_no + std::min(partition_no, excess);

	return std::make_pair(start, start + count);
}

}
