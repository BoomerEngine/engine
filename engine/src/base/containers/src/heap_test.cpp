/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "heap.h"

DECLARE_TEST_FILE(Heap);

using namespace base;

TEST(Heap, InitEmpty)
{
	Heap<uint32_t> h;
	ASSERT_TRUE(h.empty());
	ASSERT_EQ(0, h.size());
}

TEST(Heap, InitFromData)
{
	uint32_t numbers[10] = { 7,5,3,2,11,9,4,0,1,8 };

	Heap<uint32_t> h(numbers, ARRAY_COUNT(numbers));
	ASSERT_FALSE(h.empty());
	ASSERT_EQ(10, h.size());
}

TEST(Heap, HeadOrdered)
{
	uint32_t numbers[10] = { 7,5,3,2,6,9,4,0,1,8 };

	Heap<uint32_t> h(numbers, ARRAY_COUNT(numbers));
	ASSERT_FALSE(h.empty());
	EXPECT_EQ(9, h.front());
}

TEST(Heap, PopOrdered)
{
	uint32_t numbers[10] = { 7,5,3,2,6,9,4,0,1,8 };

	Heap<uint32_t> h(numbers, ARRAY_COUNT(numbers));
	ASSERT_FALSE(h.empty());

	for (int i = 9; i >= 0; --i)
	{
		EXPECT_EQ((uint32_t)i, h.front());
		h.pop();
	}
}

TEST(Heap, BuildByPushing)
{
	uint32_t numbers[10] = { 7,5,3,2,6,9,4,0,1,8 };

	Heap<uint32_t> h;
	ASSERT_TRUE(h.empty());

	for (uint32_t i = 0; i < ARRAY_COUNT(numbers); ++i)
		h.push(numbers[i]);

	ASSERT_FALSE(h.empty());
	ASSERT_EQ(10, h.size());
	EXPECT_EQ(9, h.front());

	for (int i = 9; i >= 0; --i)
	{
		EXPECT_EQ((uint32_t)i, h.front());
		h.pop();
	}
}

TEST(Heap, DuplicatesAreHandles)
{
	uint32_t numbers[10] = { 1,2,3,4,5,5,4,3,2,1 };

	Heap<uint32_t> h;
	ASSERT_TRUE(h.empty());

	for (uint32_t i = 0; i < ARRAY_COUNT(numbers); ++i)
		h.push(numbers[i]);

	ASSERT_FALSE(h.empty());
	ASSERT_EQ(10, h.size());

	for (uint32_t i = 10; i > 0; --i)
	{
		EXPECT_EQ((i+1)/2, h.front());
		h.pop();
	}
}
