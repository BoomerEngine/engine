/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "array.h"
#include "lifetime_test.h"

DECLARE_TEST_FILE(Array);

using namespace base;

TEST(Array, InitEmpty)
{
	Array<int> a;
	EXPECT_TRUE(a.empty());
	EXPECT_TRUE(a.full());
	EXPECT_EQ(0, a.size());
	EXPECT_EQ(0, a.capacity());
}

TEST(Array, InitFromData)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
}

TEST(Array, DataPtrRead)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());

	auto ptr  = a.typedData();
	ASSERT_TRUE(ptr != nullptr);

	EXPECT_EQ(1, ptr[0]);
	EXPECT_EQ(2, ptr[1]);
	EXPECT_EQ(3, ptr[2]);
}

TEST(Array, BracketOperatorRead)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());

	EXPECT_EQ(1, a[0]);
	EXPECT_EQ(2, a[1]);
	EXPECT_EQ(3, a[2]);
}

TEST(Array, BracketOperatorWrite)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());

	EXPECT_EQ(1, a[0]);
	EXPECT_EQ(2, a[1]);
	EXPECT_EQ(3, a[2]);

	a[0] = 4;
	a[1] = 5;
	a[2] = 6;

	EXPECT_EQ(4, a[0]);
	EXPECT_EQ(5, a[1]);
	EXPECT_EQ(6, a[2]);
}

TEST(Array, DataPtrAliasesBracketOperator)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());

	auto ptr  = a.typedData();
	ASSERT_TRUE(ptr != nullptr);

	EXPECT_EQ(1, ptr[0]);
	ptr[0] = 4;
	EXPECT_EQ(4, a[0]);

	a[1] = 42;
	EXPECT_EQ(42, ptr[1]);
}

TEST(Array, InitFromDataMakesACopy)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());

	data[0] = 4;
	data[1] = 5;
	data[2] = 6;

	EXPECT_EQ(1, a[0]);
	EXPECT_EQ(2, a[1]);
	EXPECT_EQ(3, a[2]);
}

TEST(Array, FrontIsFirst)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	EXPECT_EQ(1, a.front());
}

TEST(Array, BackIsLast)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	EXPECT_EQ(3, a.back());
}

TEST(Array, CopyCtor)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));

	Array<uint32_t> b(a);
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());
	
	EXPECT_EQ(1, b[0]);
	EXPECT_EQ(2, b[1]);
	EXPECT_EQ(3, b[2]);
}

TEST(Array, CopyCtorMakesCopy)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));

	Array<uint32_t> b(a);

	a[0] = 4;
	a[1] = 5;
	a[2] = 6;

	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());

	EXPECT_EQ(1, b[0]);
	EXPECT_EQ(2, b[1]);
	EXPECT_EQ(3, b[2]);
}

TEST(Array, MoveCtor)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));

	Array<uint32_t> b(std::move(a));

	EXPECT_TRUE(a.empty());
	ASSERT_EQ(0, a.size());
	
	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());	

	EXPECT_EQ(1, b[0]);
	EXPECT_EQ(2, b[1]);
	EXPECT_EQ(3, b[2]);
}

TEST(Array, CopyAssignment)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));

	Array<uint32_t> b;
	b = a;

	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());

	EXPECT_EQ(1, b[0]);
	EXPECT_EQ(2, b[1]);
	EXPECT_EQ(3, b[2]);
}

TEST(Array, CopyAssignmentMakesCopy)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));

	Array<uint32_t> b;
	b = a;

	a[0] = 4;
	a[1] = 5;
	a[2] = 6;

	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());

	EXPECT_EQ(1, b[0]);
	EXPECT_EQ(2, b[1]);
	EXPECT_EQ(3, b[2]);
}

TEST(Array, MoveAssignment)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));

	Array<uint32_t> b;
	b = std::move(a);

	EXPECT_TRUE(a.empty());
	ASSERT_EQ(0, a.size());

	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());

	EXPECT_EQ(1, b[0]);
	EXPECT_EQ(2, b[1]);
	EXPECT_EQ(3, b[2]);
}

TEST(Array, SwapWithEmpty)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());

	Array<uint32_t> b;
	EXPECT_TRUE(b.empty());
	ASSERT_EQ(0, b.size());

	std::swap(a,b);

	EXPECT_TRUE(a.empty());
	ASSERT_EQ(0, a.size());
	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());

	EXPECT_EQ(1, b[0]);
	EXPECT_EQ(2, b[1]);
	EXPECT_EQ(3, b[2]);
}

TEST(Array, SwapTwoArrays)
{
	uint32_t data[3] = { 1,2,3 };
	uint32_t data2[3] = { 4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());

	Array<uint32_t> b(data2, ARRAY_COUNT(data2));
	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());

	std::swap(a,b);

	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	EXPECT_FALSE(b.empty());
	ASSERT_EQ(3, b.size());

	EXPECT_EQ(1, b[0]);
	EXPECT_EQ(2, b[1]);
	EXPECT_EQ(3, b[2]);

	EXPECT_EQ(4, a[0]);
	EXPECT_EQ(5, a[1]);
	EXPECT_EQ(6, a[2]);
}

TEST(Array, FrontRead)
{
	uint32_t data[3] = { 1,2,3 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_EQ(1, a.front());
}

TEST(Array, FrontWrite)
{
	uint32_t data[3] = { 1,2,3 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	a.front() = 42;
	EXPECT_EQ(42, a.front());
}

TEST(Array, BackRead)
{
	uint32_t data[3] = { 1,2,3 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_EQ(3, a.back());
}

TEST(Array, BackWrite)
{
	uint32_t data[3] = { 1,2,3 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	a.back() = 42;
	EXPECT_EQ(42, a.back());
}

TEST(Array, InsertSingleFront)
{
	uint32_t data[3] = { 1,2,3 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	a.insert(0, 42);
	EXPECT_EQ(4, a.size());
	EXPECT_EQ(42, a.front());
}

TEST(Array, InsertSingleBack)
{
	uint32_t data[3] = { 1,2,3 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	a.insert(a.size(), 42);
	EXPECT_EQ(4, a.size());
	EXPECT_EQ(42, a.back());
}

TEST(Array, InsertSingleMidle)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	a.insert(2, 42);
	ASSERT_EQ(5, a.size());
	EXPECT_EQ(1, a[0]);
	EXPECT_EQ(2, a[1]);
	EXPECT_EQ(42, a[2]);
	EXPECT_EQ(3, a[3]);
	EXPECT_EQ(4, a[4]);
}

TEST(Array, InsertMultipleFront)
{
	uint32_t data[3] = { 1,2,3 };
	uint32_t crap[2] = { 42,666 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	a.insert(0, crap, ARRAY_COUNT(crap));
	EXPECT_EQ(5, a.size());
	EXPECT_EQ(42, a[0]);
	EXPECT_EQ(666, a[1]);
	EXPECT_EQ(1, a[2]);
	EXPECT_EQ(2, a[3]);
	EXPECT_EQ(3, a[4]);
}

TEST(Array, InsertMultipleBack)
{
	uint32_t data[3] = { 1,2,3 };
	uint32_t crap[2] = { 42,666 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	a.insert(a.size(), crap, ARRAY_COUNT(crap));
	EXPECT_EQ(5, a.size());
	EXPECT_EQ(1, a[0]);
	EXPECT_EQ(2, a[1]);
	EXPECT_EQ(3, a[2]);
	EXPECT_EQ(42, a[3]);
	EXPECT_EQ(666, a[4]);
}

TEST(Array, InsertMultipleMidle)
{
	uint32_t data[4] = { 1,2,3,4 };
	uint32_t crap[2] = { 42,666 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	a.insert(2, crap, ARRAY_COUNT(crap));
	ASSERT_EQ(6, a.size());
	EXPECT_EQ(1, a[0]);
	EXPECT_EQ(2, a[1]);
	EXPECT_EQ(42, a[2]);
	EXPECT_EQ(666, a[3]);
	EXPECT_EQ(3, a[4]);
	EXPECT_EQ(4, a[5]);
}

TEST(Array, PopBack)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	EXPECT_EQ(4, a.back());
	a.popBack();
	ASSERT_EQ(3, a.size());
	EXPECT_EQ(3, a.back());
	a.popBack();
	ASSERT_EQ(2, a.size());
	EXPECT_EQ(2, a.back());
	a.popBack();
	ASSERT_EQ(1, a.size());
	EXPECT_EQ(1, a.back());
	a.popBack();
	EXPECT_TRUE(a.empty());
}

TEST(Array, PushBackSingle)
{
	Array<uint32_t> a;
	ASSERT_EQ(0, a.size());
	ASSERT_TRUE(a.empty());
	a.pushBack(1);
	ASSERT_EQ(1, a.size());
	EXPECT_EQ(1, a.back());
	a.pushBack(2);
	ASSERT_EQ(2, a.size());
	EXPECT_EQ(2, a.back());
	a.pushBack(3);
	ASSERT_EQ(3, a.size());
	EXPECT_EQ(3, a.back());
	a.pushBack(4);
	ASSERT_EQ(4, a.size());
	EXPECT_EQ(4, a.back());

	EXPECT_EQ(1, a[0]);
	EXPECT_EQ(2, a[1]);
	EXPECT_EQ(3, a[2]);
	EXPECT_EQ(4, a[3]);
}

TEST(Array, PushBackMultiple)
{
	Array<uint32_t> a;
	ASSERT_EQ(0, a.size());
	ASSERT_TRUE(a.empty());
	a.pushBack(1);
	ASSERT_EQ(1, a.size());
	ASSERT_EQ(1, a[0]);

	uint32_t crap[2] = { 42,666 };
	a.pushBack(crap, ARRAY_COUNT(crap));
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(42, a[1]);
	ASSERT_EQ(666, a[2]);
}

TEST(Array, FindIndexExisting)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	auto index0 = a.find(2);
	EXPECT_EQ(1, index0);
}

TEST(Array, FindIndexMissing)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	auto index0 = a.find(5);
	EXPECT_EQ(-1, index0);
}

TEST(Array, PushBackUniqueDoesNotAddExisting)
{
	Array<uint32_t> a;
	ASSERT_EQ(0, a.size());
	ASSERT_TRUE(a.empty());
	a.pushBack(1);
	a.pushBack(2);
	a.pushBack(3);
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);

	a.pushBackUnique(1);
	a.pushBackUnique(2);
	a.pushBackUnique(3);

	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
}

TEST(Array, PushBackUniqueAddsNotExisting)
{
	Array<uint32_t> a;
	ASSERT_EQ(0, a.size());
	ASSERT_TRUE(a.empty());
	a.pushBack(1);
	a.pushBack(2);
	a.pushBack(3);
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);

	a.pushBackUnique(4);
	a.pushBackUnique(5);
	a.pushBackUnique(6);

	ASSERT_EQ(6, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(4, a[3]);
	ASSERT_EQ(5, a[4]); 
	ASSERT_EQ(6, a[5]);
}

TEST(Array, EmplaceBack)
{
	Array<uint32_t> a;
	ASSERT_EQ(0, a.size());
	ASSERT_TRUE(a.empty());
	a.emplaceBack(1);
	a.emplaceBack(2);
	a.emplaceBack(3);
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
}

TEST(Array, EraseFrontSingle)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	a.erase(0, 1);
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(2, a[0]);
	ASSERT_EQ(3, a[1]);
	ASSERT_EQ(4, a[2]);
}

TEST(Array, EraseFrontMultiple)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	a.erase(0, 2);
	ASSERT_EQ(2, a.size());
	ASSERT_EQ(3, a[0]);
	ASSERT_EQ(4, a[1]);
}

TEST(Array, EraseBackSingle)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	a.erase(3, 1);
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
}

TEST(Array, EraseBackMultiple)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	a.erase(2, 2);
	ASSERT_EQ(2, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
}

TEST(Array, EraseMiddleSingle)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	a.erase(2, 1);
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(4, a[2]);
}

TEST(Array, EraseMiddleMultiple)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	a.erase(1, 2);
	ASSERT_EQ(2, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(4, a[1]);
}

TEST(Array, EraseAll)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	a.erase(0, 4);
	ASSERT_EQ(0, a.size());
}

TEST(Array, Clear)
{
	uint32_t data[4] = { 1,2,3,4 };
	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(4, a.size());
	a.erase(0, 4);
	ASSERT_EQ(0, a.size());
}

TEST(Array, AllocateAddsAtTheEnd)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);

	auto items  = a.allocate(3);
	ASSERT_EQ(6, a.size());

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
}

TEST(Array, AllocateAddDefaultInitialized)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);

	auto items  = a.allocate(3);
	ASSERT_EQ(6, a.size());

	/*ASSERT_EQ(0, items[0]);
	ASSERT_EQ(0, items[1]);
	ASSERT_EQ(0, items[2]);*/
}

TEST(Array, AllocateWithsAddsValueInitialized)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_FALSE(a.empty());
	ASSERT_EQ(3, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);

	auto items  = a.allocateWith(3, 42);
	ASSERT_EQ(6, a.size());

	ASSERT_EQ(42, items[0]);
	ASSERT_EQ(42, items[1]);
	ASSERT_EQ(42, items[2]);
	ASSERT_EQ(42, a[3]);
	ASSERT_EQ(42, a[4]);
	ASSERT_EQ(42, a[5]);
}

TEST(Array, RemoveSingle)
{
	uint32_t data[5] = { 1,2,3,4,5 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(5, a.size());

	auto ret = a.remove(3);
	EXPECT_TRUE(ret);

	ASSERT_EQ(4, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(4, a[2]);
	ASSERT_EQ(5, a[3]);
}

TEST(Array, RemoveSingleFailsIfDoesNotExist)
{
	uint32_t data[5] = { 1,2,3,4,5 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(5, a.size());

	auto ret = a.remove(6);
	EXPECT_FALSE(ret);

	ASSERT_EQ(5, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(4, a[3]);
	ASSERT_EQ(5, a[4]);
}

TEST(Array, RemoveMultiple)
{
	uint32_t data[6] = { 1,2,1,2,1,2 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(6, a.size());

	auto ret = a.removeAll(1);
	EXPECT_TRUE(ret);
	ASSERT_EQ(3, a.size());

	ret = a.removeAll(2);
	EXPECT_TRUE(ret);
	ASSERT_EQ(0, a.size());
}

TEST(Array, EraseUnordered)
{
	uint32_t data[6] = { 1,2,3,4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(6, a.size());

	a.eraseUnordered(1); // removes 2
	
	ASSERT_EQ(5, a.size());
	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(6, a[1]); // swapped back
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(4, a[3]);
	ASSERT_EQ(5, a[4]);	
}

TEST(Array, BeginEndIteratorSize)
{
	uint32_t data[6] = { 1,2,3,4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	EXPECT_EQ(a.size(), (uint32_t)std::distance(a.begin(), a.end()));
}

TEST(Array, BeginEndIteratorRead)
{
	uint32_t data[6] = { 1,2,3,4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(6, a.size());

	uint32_t i = 1;
	for (auto& it : a)
	{
		EXPECT_EQ(it, i);
		++i;
	}
}

TEST(Array, BeginEndIteratorWrite)
{
	uint32_t data[6] = { 1,2,3,4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(6, a.size());

	uint32_t i = 10;
	for (auto& it : a)
		it = i++;

	ASSERT_EQ(10, a[0]);
	ASSERT_EQ(11, a[1]);
	ASSERT_EQ(12, a[2]);
	ASSERT_EQ(13, a[3]);
	ASSERT_EQ(14, a[4]);
	ASSERT_EQ(15, a[5]);
}

TEST(Array, ShrinkReducesCapacity)
{
	uint32_t data[6] = { 1,2,3,4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(6, a.size());

	a.popBack();
	ASSERT_EQ(5, a.size());
	a.shrink();
	ASSERT_EQ(5, a.capacity());

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(4, a[3]);
	ASSERT_EQ(5, a[4]);
}

TEST(Array, ShrinkReleasesMemoryOnEmptyArray)
{
	uint32_t data[6] = { 1,2,3,4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(6, a.size());

	a.erase(0, 6);
	ASSERT_EQ(0, a.size());
	ASSERT_NE(0, a.capacity());
	ASSERT_TRUE(a.typedData());

	a.shrink();
	ASSERT_EQ(0, a.capacity());
	ASSERT_TRUE(!a.typedData());
}

TEST(Array, ReserveChangesCapacity)
{
	uint32_t data[5] = { 1,2,3,4,5 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(5, a.size());
	ASSERT_GE(a.capacity(), 5);

	a.reserve(100);

	ASSERT_EQ(5, a.size());
	ASSERT_GE(a.capacity(), 100);

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(4, a[3]);
	ASSERT_EQ(5, a[4]);
}

TEST(Array, ResizeInitializesDefault)
{
	Array<uint32_t> a;
	ASSERT_EQ(0, a.size());
	ASSERT_EQ(0, a.capacity());
	a.resize(5);
	ASSERT_EQ(5, a.size());
	ASSERT_GE(a.capacity(), 5);
	/*ASSERT_EQ(0, a[0]);
	ASSERT_EQ(0, a[1]);
	ASSERT_EQ(0, a[2]);
	ASSERT_EQ(0, a[3]);
	ASSERT_EQ(0, a[4]);*/
}

TEST(Array, ResizeWithInitializesValue)
{
	Array<uint32_t> a;
	ASSERT_EQ(0, a.size());
	ASSERT_EQ(0, a.capacity());
	a.resizeWith(5, 42);
	ASSERT_EQ(5, a.size());
	ASSERT_GE(a.capacity(), 5);
	ASSERT_EQ(42, a[0]);
	ASSERT_EQ(42, a[1]);
	ASSERT_EQ(42, a[2]);
	ASSERT_EQ(42, a[3]);
	ASSERT_EQ(42, a[4]);
}

TEST(Array, ResizeAddsDefaultElements)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(3, a.size());
	ASSERT_GE(a.capacity(), 3);

	a.resize(6);

	ASSERT_EQ(6, a.size());
	ASSERT_GE(a.capacity(), 6);

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	/*ASSERT_EQ(0, a[3]);
	ASSERT_EQ(0, a[4]);
	ASSERT_EQ(0, a[5]);*/
}

TEST(Array, ResizeWithAddsValueElements)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(3, a.size());
	ASSERT_GE(a.capacity(), 3);

	a.resizeWith(6, 42);

	ASSERT_EQ(6, a.size());
	ASSERT_GE(a.capacity(), 6);

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(42, a[3]);
	ASSERT_EQ(42, a[4]);
	ASSERT_EQ(42, a[5]);
}

TEST(Array, ResizeRemovesElement)
{
	uint32_t data[6] = { 1,2,3,4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(6, a.size());
	ASSERT_GE(a.capacity(), 6);

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(4, a[3]);
	ASSERT_EQ(5, a[4]);
	ASSERT_EQ(6, a[5]);

	a.resize(3);

	ASSERT_EQ(3, a.size());
	ASSERT_GE(a.capacity(), 3);

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
}

TEST(Array, ResizeClearsArray)
{
	uint32_t data[6] = { 1,2,3,4,5,6 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(6, a.size());
	ASSERT_GE(a.capacity(), 6);

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(4, a[3]);
	ASSERT_EQ(5, a[4]);
	ASSERT_EQ(6, a[5]);

	a.resize(0);

	ASSERT_EQ(0, a.size());
	ASSERT_GE(a.capacity(), 0);
	ASSERT_TRUE(a.empty());
}

TEST(Array, PrepareInitializesEmptyElements)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(3, a.size());

	a.prepare(10);
	ASSERT_EQ(10, a.size());

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	/*ASSERT_EQ(0, a[4]);
	ASSERT_EQ(0, a[9]);*/
}

TEST(Array, PrepareWithInitializesVelueElements)
{
	uint32_t data[3] = { 1,2,3 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(3, a.size());

	a.prepareWith(10, 42);
	ASSERT_EQ(10, a.size());

	ASSERT_EQ(1, a[0]);
	ASSERT_EQ(2, a[1]);
	ASSERT_EQ(3, a[2]);
	ASSERT_EQ(42, a[4]);
	ASSERT_EQ(42, a[9]);
}

TEST(Array, PrepareDoesNotMakeArraySmaller)
{
	uint32_t data[5] = { 1,2,3,4,5 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(5, a.size());

	a.prepare(2);
	ASSERT_EQ(5, a.size());
}

TEST(Array, CompareFailsOnDifferentSize)
{
	Array<uint32_t> a, b;
	a.resizeWith(10, 0);
	b.resizeWith(11, 0);
	EXPECT_FALSE(a == b);
	EXPECT_TRUE(a != b);
}

TEST(Array, CompareSameContent)
{
	Array<uint32_t> a, b;
	a.resizeWith(10, 0);
	b.resizeWith(10, 0);
	EXPECT_TRUE(a == b);
	EXPECT_FALSE(a != b);
}

TEST(Array, CompareDifferentContent)
{
	Array<uint32_t> a, b;
	a.resizeWith(10, 0);
	b.resizeWith(10, 1);
	EXPECT_FALSE(a == b);
	EXPECT_TRUE(a != b);
}

TEST(Array, CompareDifferentContentOneByte)
{
	Array<uint32_t> a, b;
	a.resizeWith(10, 0);
	b.resizeWith(10, 0);
	b[9] = 42;
	EXPECT_FALSE(a == b);
	EXPECT_TRUE(a != b);
}

TEST(Array, CreateDynamicBufferMakesCopy)
{
	uint32_t data[5] = { 1,2,3,4,5 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(5, a.size());

	auto buf = a.createBuffer();
	ASSERT_TRUE(buf != nullptr);
	ASSERT_EQ(a.dataSize(), buf.size());

	auto ptr  = (const uint32_t*)buf.data();
	EXPECT_EQ(1, ptr[0]);
	EXPECT_EQ(2, ptr[1]);
	EXPECT_EQ(3, ptr[2]);
	EXPECT_EQ(4, ptr[3]);
	EXPECT_EQ(5, ptr[4]);

	a[2] = 42;

	EXPECT_EQ(3, ptr[2]);
}

TEST(Array, CreateInlinedBufferReferencesOriginal)
{
	uint32_t data[5] = { 1,2,3,4,5 };

	Array<uint32_t> a(data, ARRAY_COUNT(data));
	ASSERT_EQ(5, a.size());

	auto buf = a.createAliasedBuffer();
	ASSERT_TRUE(buf != nullptr);
	ASSERT_EQ(a.dataSize(), buf.size());

	auto ptr  = (const uint32_t*)buf.data();
	EXPECT_EQ(1, ptr[0]);
	EXPECT_EQ(2, ptr[1]);
	EXPECT_EQ(3, ptr[2]);
	EXPECT_EQ(4, ptr[3]);
	EXPECT_EQ(5, ptr[4]);

	a[2] = 42;

	EXPECT_EQ(42, ptr[2]);
}

//--

class ArrayObjectTesting : public tests::SimpleTrackerTest
{
};

TEST_F(ArrayObjectTesting, ReserveDoesNotCreate)
{
	{
		Array<tests::SimpleTracker> a;
		a.reserve(10);
		EXPECT_EQ(0, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumValueConstructed);
	}
	EXPECT_EQ(0, tests::SimpleTracker::NumDestructed);
}

TEST_F(ArrayObjectTesting, ResizeCreatesDefault)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(5);
		EXPECT_EQ(5, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumValueConstructed);
	}
	EXPECT_EQ(5, tests::SimpleTracker::NumDestructed);
}

TEST_F(ArrayObjectTesting, ResizeWithCreatesValueInitialized)
{
	{
		Array<tests::SimpleTracker> a;
		tests::SimpleTracker value(42);
		a.resizeWith(5, value);
		EXPECT_EQ(0, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(1, tests::SimpleTracker::NumValueConstructed);
		EXPECT_EQ(5, tests::SimpleTracker::NumCopyConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
		EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
		EXPECT_EQ(0, tests::SimpleTracker::NumMoveConstructed);
	}
	EXPECT_EQ(6, tests::SimpleTracker::NumDestructed);
}

TEST_F(ArrayObjectTesting, ResizeDoestNotCopy)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(10);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		a.resize(20);
		EXPECT_EQ(20, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumDestructed);
	}
	EXPECT_EQ(20, tests::SimpleTracker::NumDestructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyConstructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveConstructed);
}

TEST_F(ArrayObjectTesting, ResizeCanCreateElements)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(10);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		a.resize(20);
		EXPECT_EQ(20, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumDestructed);
		a.resize(50);
		EXPECT_EQ(50, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumDestructed);
	}
	EXPECT_EQ(50, tests::SimpleTracker::NumDestructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyConstructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveConstructed);
}

TEST_F(ArrayObjectTesting, ResizeCanFreeElements)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(10);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		a.resize(5);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(5, tests::SimpleTracker::NumDestructed);
		a.resize(1);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(9, tests::SimpleTracker::NumDestructed);

	}
	EXPECT_EQ(10, tests::SimpleTracker::NumDestructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyConstructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveConstructed);
}

TEST_F(ArrayObjectTesting, ClearDestroysElementsAndMemory)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(10);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumDestructed);
		a.clear();
		EXPECT_EQ(10, tests::SimpleTracker::NumDestructed);
		EXPECT_EQ(0, a.capacity());
		EXPECT_TRUE(!a.typedData());
	}
	EXPECT_EQ(10, tests::SimpleTracker::NumDestructed);
}

TEST_F(ArrayObjectTesting, ResetDestroysElementsNotMemory)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(10);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumDestructed);
		a.reset();
		EXPECT_EQ(10, tests::SimpleTracker::NumDestructed);
		EXPECT_NE(0, a.capacity());
		EXPECT_TRUE(a.typedData());
	}
	EXPECT_EQ(10, tests::SimpleTracker::NumDestructed);
}

TEST_F(ArrayObjectTesting, ArrayFromDataMakesCopy)
{
	{
		tests::SimpleTracker b[3];
		EXPECT_EQ(3, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumCopyConstructed);

		Array<tests::SimpleTracker> a(b, 3);
		EXPECT_EQ(3, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(3, tests::SimpleTracker::NumCopyConstructed);
	}

	EXPECT_EQ(6, tests::SimpleTracker::NumDestructed);
}

TEST_F(ArrayObjectTesting, ArrayFromOtherArrayMakesCopy)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(3);
		EXPECT_EQ(3, tests::SimpleTracker::NumDefaultConstructed);

		Array<tests::SimpleTracker> b(a);
		EXPECT_EQ(3, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(3, tests::SimpleTracker::NumCopyConstructed);
		EXPECT_EQ(3, a.size());
	}

	EXPECT_EQ(6, tests::SimpleTracker::NumDestructed);
}

TEST_F(ArrayObjectTesting, ArraySwappingDoesNotCopyOrMoveObjects)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(3);
		EXPECT_EQ(3, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(3, a.size());

		Array<tests::SimpleTracker> b;
		b.resize(3);
		EXPECT_EQ(6, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(3, b.size());

		std::swap(a,b);
		
		EXPECT_EQ(6, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumCopyConstructed);		
	}

	EXPECT_EQ(6, tests::SimpleTracker::NumDestructed);
}

TEST_F(ArrayObjectTesting, EmplaceDoesNotCopy)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(3);
		EXPECT_EQ(3, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(3, a.size());

		tests::SimpleTracker b;
		a.emplaceBack(std::move(b));
		EXPECT_EQ(4, a.size());
		EXPECT_EQ(4, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumCopyConstructed);
		EXPECT_EQ(1, tests::SimpleTracker::NumMoveConstructed);
	}

	EXPECT_EQ(4, tests::SimpleTracker::NumDestructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
}

TEST_F(ArrayObjectTesting, PushBackDoesCopy)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(3);
		EXPECT_EQ(3, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(3, a.size());

		tests::SimpleTracker b;
		a.pushBack(b);
		EXPECT_EQ(4, a.size());
		EXPECT_EQ(4, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(1, tests::SimpleTracker::NumCopyConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumMoveConstructed);
	}

	EXPECT_EQ(5, tests::SimpleTracker::NumDestructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
}

TEST_F(ArrayObjectTesting, InsertDoesNotCopyElements)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(10);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(10, a.size());

		tests::SimpleTracker b;
		a.insert(5, b);
		EXPECT_EQ(11, a.size());
		EXPECT_EQ(11, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(1, tests::SimpleTracker::NumCopyConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumMoveConstructed);
	}

	EXPECT_EQ(12, tests::SimpleTracker::NumDestructed); // +1 for the b
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
}

TEST_F(ArrayObjectTesting, PopBackDestroysElements)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(10);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumDestructed);
		EXPECT_EQ(10, a.size());

		a.popBack();
		EXPECT_EQ(1, tests::SimpleTracker::NumDestructed);

		a.popBack();
		EXPECT_EQ(2, tests::SimpleTracker::NumDestructed);

		a.popBack();
		EXPECT_EQ(3, tests::SimpleTracker::NumDestructed);
		EXPECT_EQ(7, a.size());
	}

	EXPECT_EQ(10, tests::SimpleTracker::NumDestructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
}

TEST_F(ArrayObjectTesting, RemoveDestroysRemoved)
{
	{
		Array<tests::SimpleTracker> a;
		a.resize(10);
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(10, a.size());

		a.erase(5, 2);
		EXPECT_EQ(8, a.size());
		EXPECT_EQ(10, tests::SimpleTracker::NumDefaultConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumCopyConstructed);
		EXPECT_EQ(0, tests::SimpleTracker::NumMoveConstructed);
		EXPECT_EQ(2, tests::SimpleTracker::NumDestructed);
	}

	EXPECT_EQ(10, tests::SimpleTracker::NumDestructed); 
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveAssigned);
	EXPECT_EQ(0, tests::SimpleTracker::NumCopyConstructed);
	EXPECT_EQ(0, tests::SimpleTracker::NumMoveConstructed);
}
























