/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "base/memory/include/structurePool.h"

DECLARE_TEST_FILE(StructurePool);

using namespace base;
using namespace base::mem;

namespace test
{

	struct SmallData
	{
		uint8_t m_data[32];
	};

	TYPE_ALIGN(256, struct) AlignedElement
	{
		uint8_t m_data[32];
	};

} // test

TEST(StructurePool, Empty)
{
	StructurePool<test::SmallData> pool;
	EXPECT_EQ(0, pool.size());
	//EXPECT_EQ(0, pool.capacity());
	//EXPECT_TRUE(pool.empty());
}

TEST(StructurePool, AllocatesSingle)
{
	StructurePool<test::SmallData> pool;

	auto elem  = pool.create();
	EXPECT_EQ(1, pool.size());

	pool.free(elem);
	EXPECT_EQ(0, pool.size());
}

TEST(StructurePool, AllocatesDifferent)
{
    StructurePool<test::SmallData> pool;

    auto elem = pool.create();
    auto elem2 = pool.create();
    EXPECT_NE(elem, elem2);

    pool.free(elem);
    pool.free(elem2);
}

TEST(StructurePool, AllocatesMultiple)
{
	StructurePool<test::SmallData> pool;

	auto elem  = pool.create();
	EXPECT_EQ(1, pool.size());

	auto elem2  = pool.create();
	EXPECT_EQ(2, pool.size());

    pool.free(elem);
    pool.free(elem2);
    EXPECT_EQ(0, pool.size());
}

TEST(StructurePool, AlignedElement)
{
	StructurePool<test::AlignedElement> pool;

	auto elem  = pool.create();
	EXPECT_EQ(0, ((uint64_t)elem) & 255);

    pool.free(elem);
}

