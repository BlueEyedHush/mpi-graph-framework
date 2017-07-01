//
// Created by blueeyedhush on 01.07.17.
//

#include <gtest/gtest.h>
#include <utils/IndexPartitioner.h>

TEST(IndexPartitioner, GetRangeWithoutExcess) {
	int N = 20;
	int P = 5;

	auto p = IndexPartitioner::get_range_for_partition(N, P, 0);
	ASSERT_EQ(p.first, 0);
	ASSERT_EQ(p.second, 4);

	p = IndexPartitioner::get_range_for_partition(N, P, 1);
	ASSERT_EQ(p.first, 4);
	ASSERT_EQ(p.second, 8);

	p = IndexPartitioner::get_range_for_partition(N, P, 2);
	ASSERT_EQ(p.first, 8);
	ASSERT_EQ(p.second, 12);

	p = IndexPartitioner::get_range_for_partition(N, P, 3);
	ASSERT_EQ(p.first, 12);
	ASSERT_EQ(p.second, 16);

	p = IndexPartitioner::get_range_for_partition(N, P, 4);
	ASSERT_EQ(p.first, 16);
	ASSERT_EQ(p.second, 20);
}

TEST(IndexPartitioner, GetRangeWithExcess) {
	int N = 14;
	int P = 3;

	auto p = IndexPartitioner::get_range_for_partition(N, P, 0);
	ASSERT_EQ(p.first, 0);
	ASSERT_EQ(p.second, 5);

	p = IndexPartitioner::get_range_for_partition(N, P, 1);
	ASSERT_EQ(p.first, 5);
	ASSERT_EQ(p.second, 10);

	p = IndexPartitioner::get_range_for_partition(N, P, 2);
	ASSERT_EQ(p.first, 10);
	ASSERT_EQ(p.second, 14);
}

TEST(IndexPartitioner, IdentifyPartitionNoExcess) {
	int N = 12;
	int P = 4;

	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 0), 0);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 2), 0);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 3), 1);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 5), 1);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 6), 2);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 11), 3);
}

TEST(IndexPartitioner, IdentifyPartitionWhenExcess) {
	int N = 14;
	int P = 4;

	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 0), 0);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 2), 0);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 3), 0);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 4), 1);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 7), 1);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 8), 2);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 10), 2);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 11), 3);
	ASSERT_EQ(IndexPartitioner::get_partition_from_index(N, P, 13), 3);
}