/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: tests #]
***/

#include "build.h"
#include "lifetime_test.h"

#include "base/test/include/gtest/gtest.h"

using namespace tests;

DECLARE_TEST_FILE(SimpleTracker);

TEST_F(SimpleTrackerTest, EmptyState)
{
	EXPECT_EQ(SimpleTracker::NumCopyAssigned, 0);
	EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 0);
	EXPECT_EQ(SimpleTracker::NumValueConstructed, 0);
	EXPECT_EQ(SimpleTracker::NumCopyConstructed, 0);
	EXPECT_EQ(SimpleTracker::NumMoveConstructed, 0);
	EXPECT_EQ(SimpleTracker::NumCopyAssigned, 0);
	EXPECT_EQ(SimpleTracker::NumMoveAssigned, 0);
	EXPECT_EQ(SimpleTracker::NumDestructed, 0);
}

TEST_F(SimpleTrackerTest, SingleObject)
{
	{
		SimpleTracker o;
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 1);
	}
	EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 1);
	EXPECT_EQ(SimpleTracker::NumDestructed, 1);
}

TEST_F(SimpleTrackerTest, SingleObjectFromValue)
{
	{
		SimpleTracker o(42);
		EXPECT_EQ(SimpleTracker::NumValueConstructed, 1);
	}
	EXPECT_EQ(SimpleTracker::NumValueConstructed, 1);
	EXPECT_EQ(SimpleTracker::NumDestructed, 1);
}

TEST_F(SimpleTrackerTest, ObjectCopyConstruct)
{
	{
		SimpleTracker o;
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 1);

		SimpleTracker b(o);
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 1);
		EXPECT_EQ(SimpleTracker::NumCopyConstructed, 1);
	}
	EXPECT_EQ(SimpleTracker::NumDestructed, 2);
}

TEST_F(SimpleTrackerTest, ObjectCopyAssign)
{
	{
		SimpleTracker o;
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 1);

		SimpleTracker b;
		b = o;
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 2);
		EXPECT_EQ(SimpleTracker::NumCopyAssigned, 1);
	}
	EXPECT_EQ(SimpleTracker::NumDestructed, 2);
}

TEST_F(SimpleTrackerTest, ObjectMoveConstruct)
{
	{
		SimpleTracker o;
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 1);

		SimpleTracker b(std::move(o));
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 1);
		EXPECT_EQ(SimpleTracker::NumCopyAssigned, 0);
		EXPECT_EQ(SimpleTracker::NumMoveAssigned, 0);
		EXPECT_EQ(SimpleTracker::NumMoveConstructed, 1);
		EXPECT_EQ(SimpleTracker::NumCopyConstructed, 0);
	}
	EXPECT_EQ(SimpleTracker::NumDestructed, 1);
}

TEST_F(SimpleTrackerTest, ObjectMoveAssigned)
{
	{
		SimpleTracker o;
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 1);

		SimpleTracker b;
		b = std::move(o);
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 2);
		EXPECT_EQ(SimpleTracker::NumMoveAssigned, 1);
		EXPECT_EQ(SimpleTracker::NumCopyAssigned, 0);
		EXPECT_EQ(SimpleTracker::NumMoveConstructed, 0);
		EXPECT_EQ(SimpleTracker::NumCopyConstructed, 0);
	}
	EXPECT_EQ(SimpleTracker::NumDestructed, 2);
}

TEST_F(SimpleTrackerTest, SimpleArray)
{
	{
		SimpleTracker o[3];
		EXPECT_EQ(SimpleTracker::NumDefaultConstructed, 3);
	}
	EXPECT_EQ(SimpleTracker::NumDestructed, 3);
}