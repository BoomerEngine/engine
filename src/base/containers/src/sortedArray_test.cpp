/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "sortedArray.h"

DECLARE_TEST_FILE(SortedArray);

using namespace base;

TEST(SortedArray, InitFromData)
{
	uint32_t numbers[10] = { 7,5,3,2,6,9,4,0,1,8 };

	SortedArray<uint32_t> data(numbers, ARRAY_COUNT(numbers));
	EXPECT_EQ(10, data.size());

	for (uint32_t i = 1; i < data.size(); ++i)
	{
		EXPECT_LT(data[i - 1], data[i]);
	}
}

TEST(SortedArray, InitFromDuiplicateData)
{
	uint32_t numbers[20] = { 7,5,3,2,6,9,4,0,1,8,5,9,6,3,4,0,2,7,8,1 };

	SortedArray<uint32_t> data(numbers, ARRAY_COUNT(numbers));
	EXPECT_EQ(10, data.size());

	for (uint32_t i = 1; i < data.size(); ++i)
	{
		EXPECT_LT(data[i - 1], data[i]);
	}
}


TEST(SortedArray, RemoveExisting)
{
	uint32_t numbers[10] = { 7,5,3,2,6,9,4,0,1,8 };

	SortedArray<uint32_t> data(numbers, ARRAY_COUNT(numbers));
	EXPECT_EQ(10, data.size());

	auto flag = data.remove(5);
	EXPECT_TRUE(flag);
	EXPECT_EQ(9, data.size());

	for (uint32_t i = 1; i < data.size(); ++i)
	{
		EXPECT_LT(data[i - 1], data[i]);
	}
}

TEST(SortedArray, RemoveNonExistingFails)
{
	uint32_t numbers[10] = { 7,5,3,2,6,9,4,0,1,8 };

	SortedArray<uint32_t> data(numbers, ARRAY_COUNT(numbers));
	EXPECT_EQ(10, data.size());

	auto flag = data.remove(15);
	EXPECT_FALSE(flag);
	EXPECT_EQ(10, data.size());

	for (uint32_t i = 1; i < data.size(); ++i)
	{
		EXPECT_LT(data[i - 1], data[i]);
	}
}

TEST(SortedArray, InsertAtEndWorks)
{
	uint32_t numbers[10] = { 7,5,3,2,6,9,4,0,1,8 };

	SortedArray<uint32_t> data(numbers, ARRAY_COUNT(numbers));
	EXPECT_EQ(10, data.size());

	auto flag = data.insert(15);
	EXPECT_TRUE(flag);
	EXPECT_EQ(11, data.size());
	EXPECT_EQ(15, data.back());

	for (uint32_t i = 1; i < data.size(); ++i)
	{
		EXPECT_LT(data[i - 1], data[i]);
	}
}

TEST(SortedArray, InsertDuplicateFails)
{
	uint32_t numbers[10] = { 7,5,3,2,6,9,4,0,1,8 };

	SortedArray<uint32_t> data(numbers, ARRAY_COUNT(numbers));
	EXPECT_EQ(10, data.size());

	auto flag = data.insert(5);
	EXPECT_FALSE(flag);
	EXPECT_EQ(10, data.size());

	for (uint32_t i = 1; i < data.size(); ++i)
	{
		EXPECT_LT(data[i - 1], data[i]);
	}
}

TEST(SortedArray, InsertMiddleWorks)
{
	uint32_t numbers[10] = { 7,5,3,2,11,9,4,0,1,8 };

	SortedArray<uint32_t> data(numbers, ARRAY_COUNT(numbers));
	EXPECT_EQ(10, data.size());

	auto flag = data.insert(6);
	EXPECT_TRUE(flag);
	EXPECT_EQ(11, data.size());

	for (uint32_t i = 1; i < data.size(); ++i)
	{
		EXPECT_LT(data[i - 1], data[i]);
	}
}