/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"

#include "bitSet.h"

using namespace base;

DECLARE_TEST_FILE(BitSet);

TEST(BitSet, InitEmpty)
{
	BitSet b;
	EXPECT_EQ(0, b.size());
	EXPECT_EQ(0, b.capcity());
}

TEST(BitSet, InitZeros)
{
	BitSet b(10, EBitStateZero::ZERO);
	EXPECT_EQ(10, b.size());
	EXPECT_FALSE(b[0]);
	EXPECT_FALSE(b[9]);
}

TEST(BitSet, InitOnes)
{
	BitSet b(10, EBitStateOne::ONE);
	EXPECT_EQ(10, b.size());
	EXPECT_TRUE(b[0]);
	EXPECT_TRUE(b[9]);
}

