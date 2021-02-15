/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"
#include "hashMap.h"

DECLARE_TEST_FILE(HashMap);

using namespace base;

typedef base::HashMap<int, int> TestIntMap;

TEST(HashMap, Empty)
{
    TestIntMap x;
    EXPECT_TRUE(x.empty());
    EXPECT_EQ(0U, x.size());
}

TEST(HashMap, BuildSmall)
{
    TestIntMap x;

    for (uint32_t i=1; i<16; ++i)
        x.set(i, i*i);

    EXPECT_FALSE(x.empty());
    EXPECT_EQ(15U, x.size());
}

TEST(HashMap, BuildMedium)
{
    TestIntMap x;

    for (uint32_t i=0; i<256; ++i)
        x.set(i, i*2);

    EXPECT_FALSE(x.empty());
    EXPECT_EQ(256U, x.size());
}

TEST(HashMap, BuildLarge)
{
    TestIntMap x;

    for (uint32_t i=0; i<65536; ++i)
        x.set(i, i*2);

    EXPECT_FALSE(x.empty());
    EXPECT_EQ(65536U, x.size());
}

TEST(HashMap, IterateKeys)
{
    TestIntMap x;

    for (int i=1; i<16; ++i)
        x.set(i, i*i);

    int i=1;
    for (auto& key : x.keys())
    {
        EXPECT_EQ(i, key);
        i += 1;
    }
}

TEST(HashMap, IterateValues)
{
    TestIntMap x;

    for (int i=1; i<16; ++i)
        x.set(i, i*i);

    int i=1;
    for (auto& val : x.values())
    {
        EXPECT_EQ(i*i, val);
        i += 1;
    }
}

TEST(HashMap, Foreach)
{
    TestIntMap x;

    for (const auto& pair : x.pairs())
    {
        EXPECT_EQ(pair.value, pair.key * pair.key);
    }
}

TEST(HashMap, LinearRegimeFind)
{
    TestIntMap x;

    for (uint32_t i=1; i<16; ++i)
        x.set(i, i*i);

    EXPECT_FALSE(x.empty());
    EXPECT_EQ(15U, x.size());
    EXPECT_EQ(1, x.findSafe(1));
    EXPECT_EQ(4, x.findSafe(2));
    EXPECT_EQ(9, x.findSafe(3));
    EXPECT_EQ(16, x.findSafe(4));
    EXPECT_EQ(25, x.findSafe(5));
    EXPECT_EQ(36, x.findSafe(6));
    EXPECT_EQ(49, x.findSafe(7));
    EXPECT_EQ(64, x.findSafe(8));
    EXPECT_EQ(81, x.findSafe(9));
    EXPECT_EQ(100, x.findSafe(10));
    EXPECT_EQ(121, x.findSafe(11));
    EXPECT_EQ(144, x.findSafe(12));
    EXPECT_EQ(169, x.findSafe(13));
    EXPECT_EQ(196, x.findSafe(14));
    EXPECT_EQ(225, x.findSafe(15));
}

TEST(HashMap, LinearRegimeReplace)
{
    TestIntMap x;

    for (uint32_t i=1; i<16; ++i)
        x.set(i, i*i);

    EXPECT_EQ(15U, x.size());
    EXPECT_EQ(81, x.findSafe(9));

    x.set(9, 666);
    EXPECT_EQ(15U, x.size());

    EXPECT_EQ(666, x.findSafe(9));
}

TEST(HashMap, LinearRegimeClear)
{
    TestIntMap x;

    for (uint32_t i=1; i<16; ++i)
        x.set(i, i*i);

    EXPECT_EQ(15U, x.size());
    EXPECT_EQ(81, x.findSafe(9));

    x.clear();
    EXPECT_EQ(0U, x.size());
    EXPECT_EQ(0, x.findSafe(9));
}


TEST(HashMap, LinearRegimeRemove)
{
    TestIntMap x;

    for (uint32_t i=1; i<16; ++i)
        x.set(i, i*i);

    EXPECT_EQ(15U, x.size());
    EXPECT_EQ(81, x.findSafe(9));

    x.remove(9);
    EXPECT_EQ(14U, x.size());
    EXPECT_EQ(0, x.findSafe(9));
}

TEST(HashMap, LinearRegimeValueReplace)
{
    TestIntMap x;

    for (uint32_t i=1; i<16; ++i)
        x.set(i, i*i);

    EXPECT_EQ(15U, x.size());

    auto val  = x.find(9);
    ASSERT_TRUE(val != nullptr);
    EXPECT_EQ(81, *val);

    *val = 666;

    EXPECT_EQ(666, x.findSafe(9));
}

static const auto HASHMAP_PERF_ITERATIONS = 20;

TEST(HashMap, Perf_Build10k)
{
    Array<int> keys;

    static const auto size = 100000;

    {
        srand(0);
        keys.resize(size);
        for (int i = 0; i < size; ++i)
            keys[i] = rand();
    }

    TimingStatistics stats;
    for (uint32_t run = 0; run < HASHMAP_PERF_ITERATIONS; ++run)
    {
        HashMap<int, int> x;
        x.reserve(size);

        {
            ScopeTimer timer;
            for (uint32_t i = 0; i < size; ++i)
                x.set(keys[i], i);

            stats.update(timer.timeElapsed());
        }
    }

    TRACE_WARNING("HashMap Build10k: {} avg, {} dev", TimeInterval(stats.mean()), TimeInterval(stats.variance()));
}

TEST(HashMap, Perf_StdBuild10k)
{
    Array<int> keys;

    static const auto size = 100000;

    {
        srand(0);
        keys.resize(size);
        for (int i = 0; i < size; ++i)
            keys[i] = rand();
    }

    TimingStatistics stats;
    for (uint32_t run = 0; run < HASHMAP_PERF_ITERATIONS; ++run)
    {
        std::unordered_map<int, int> x;
        x.reserve(size);

        {
            ScopeTimer timer;
            for (uint32_t i = 0; i < size; ++i)
                x[keys[i]] = i;

            stats.update(timer.timeElapsed());
        }
    }

    TRACE_WARNING("StdHashMap Build10k: {} avg, {} dev", TimeInterval(stats.mean()), TimeInterval(stats.variance()));
}


TEST(HashMap, Perf_Find10k)
{
    Array<int> keys;

    static const auto size = 100000;

    {
        srand(0);
        keys.resize(size);
        for (int i = 0; i < size; ++i)
            keys[i] = rand();
    }

    HashMap<int, int> x;

    {
        x.reserve(size);
        for (uint32_t i = 0; i < size; ++i)
            x.set(keys[i], i);
    }

    TimingStatistics stats;
    uint32_t totalFound = 0;
    for (uint32_t run = 0; run < HASHMAP_PERF_ITERATIONS; ++run)
    {
        totalFound = 0;

        {
            ScopeTimer timer;

            for (uint32_t i = 0; i < size; ++i)
                totalFound += x.find(i) != nullptr;

            stats.update(timer.timeElapsed());
        }
    }

    TRACE_WARNING("HashMap Find10k: {} avg, {} dev ({}, {})", TimeInterval(stats.mean()), TimeInterval(stats.variance()), x.size(), totalFound);
}

TEST(HashMap, Perf_StdFind10k)
{
    Array<int> keys;

    static const auto size = 100000;

    {
        srand(0);
        keys.resize(size);
        for (int i = 0; i < size; ++i)
            keys[i] = rand();
    }

    std::unordered_map<int, int> x;

    {
        x.reserve(size);
        for (uint32_t i = 0; i < size; ++i)
            x[keys[i]] = i;
    }

    TimingStatistics stats;
    uint32_t totalFound = 0;
    for (uint32_t run = 0; run < HASHMAP_PERF_ITERATIONS; ++run)
    {
        totalFound = 0;

        {
            ScopeTimer timer;

            for (uint32_t i = 0; i < size; ++i)
            {
                auto it = x.find(i);
                if (it != x.end())
                    totalFound += 1;
            }

            stats.update(timer.timeElapsed());
        }
    }

    TRACE_WARNING("StdHashMap Find10k: {} avg, {} dev ({}, {})", TimeInterval(stats.mean()), TimeInterval(stats.variance()), x.size(), totalFound);
}