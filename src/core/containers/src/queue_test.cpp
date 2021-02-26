/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "core/test/include/gtest/gtest.h"
#include "queue.h"

DECLARE_TEST_FILE(Queue);

BEGIN_BOOMER_NAMESPACE()

TEST(Queue, InitialEmpty)
{
	Queue<int> q;
	ASSERT_TRUE(q.empty());
}

TEST(Queue, InitialNotFull)
{
	Queue<int> q;
	ASSERT_FALSE(q.full());
}

TEST(Queue, InitialSizeZero)
{
	Queue<int> q;
	ASSERT_EQ(0, q.size());
}

TEST(Queue, AddingOneNonEmpty)
{
	Queue<int> q;
	q.push(1);
	ASSERT_FALSE(q.empty());
}

TEST(Queue, AddingOneValidSize)
{
	Queue<int> q;
	q.push(1);
	ASSERT_EQ(1, q.size());
}

TEST(Queue, PushPop)
{
	Queue<int> q;
	q.push(42);
	ASSERT_EQ(42, q.top());
	q.pop();
	ASSERT_EQ(0, q.size());
	ASSERT_TRUE(q.empty());
}

TEST(Queue, Clear)
{
	Queue<int> q;
	q.push(42);
	q.push(123);
	ASSERT_EQ(2, q.size());
	q.clear();
	ASSERT_EQ(0, q.size());
	ASSERT_TRUE(q.empty());
	ASSERT_FALSE(q.full());
}

TEST(Queue, NotFullWhileInCapactiy)
{
	Queue<int> q;
	
	q.push(42);
	while (q.size() < q.capacity())
		q.push(42);

	ASSERT_TRUE(q.full());
}

TEST(Queue, PushPopSmall)
{
	Queue<int> q;

	uint32_t i = 0;
	q.push(i++);
	while (q.size() < q.capacity())
		q.push(i++);

	ASSERT_EQ(i, q.size());

	uint32_t j = 0;
	while (!q.empty())
	{
		auto z = q.top();
		EXPECT_EQ(z, j);
		j += 1;
		q.pop();
	}

	ASSERT_TRUE(q.empty());
}

TEST(Queue, PushPopSmallBatches)
{
	Queue<int> q;

	uint32_t i = 0;
	q.push(i++);
	while (q.size() < q.capacity())
		q.push(i++);

	ASSERT_EQ(i, q.size());

	srand(0);

	uint32_t j = 0;
	for (uint32_t k = 0; k < 100; ++k)
	{
		auto prevSize = q.size();
		auto popCount = 1 + rand() % prevSize;

		for (uint32_t l = 0; l < popCount; ++l)
		{
			ASSERT_FALSE(q.empty());
			auto z = q.top();
			EXPECT_EQ(z, j);
			j += 1;
			q.pop();
		}

		ASSERT_EQ(q.size(), prevSize - popCount);

		while (q.size() < q.capacity())
			q.push(i++);
	}
}

TEST(Queue, PushPopLarge)
{
	Queue<int> q;

	for (uint32_t i = 0; i < 4096; ++i)
		q.push(i);
	

	ASSERT_EQ(4096, q.size());

	uint32_t j = 0;
	while (!q.empty())
	{
		auto z = q.top();
		EXPECT_EQ(z, j);
		j += 1;
		q.pop();
	}

	ASSERT_TRUE(q.empty());
}

TEST(Queue, PushPopLargeBatches)
{
	Queue<int> q;

	auto MAX = 4096U;

	uint32_t i = 0;
	q.push(i++);
	while (q.size() < MAX)
		q.push(i++);

	ASSERT_EQ(i, q.size());

	srand(0);

	uint32_t j = 0;
	for (uint32_t k = 0; k < 100; ++k)
	{
		auto prevSize = q.size();
		auto popCount = 1 + rand() % prevSize;

		for (uint32_t l = 0; l < popCount; ++l)
		{
			ASSERT_FALSE(q.empty());
			auto z = q.top();
			EXPECT_EQ(z, j);
			j += 1;
			q.pop();
		}

		ASSERT_EQ(q.size(), prevSize - popCount);

		while (q.size() < MAX)
			q.push(i++);
	}
}

END_BOOMER_NAMESPACE()