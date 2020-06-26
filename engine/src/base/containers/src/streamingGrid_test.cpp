/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: tests #]
***/

#include "build.h"

#include "base/test/include/gtest/gtest.h"

#include "streamingGrid.h"
#include "streamingGridHelpers.h"

using namespace base;

DECLARE_TEST_FILE(StreamingGrid);

#define NUM_GRID_LEVELS 8
#define NUM_GRID_BUCKETS 10000

/// step of the stateless generator
static INLINE uint64_t StatelessNextUint64(uint64_t& state)
{
#ifdef PLATFORM_MSVC
    state += UINT64_C(0x60bee2bee120fc15);
    uint64_t tmp;
    tmp = (uint64_t)state * UINT64_C(0xa3b195354a39b70d);
    uint64_t m1 = tmp;
    tmp = (uint64_t)m1 * UINT64_C(0x1b03738712fad5c9);
    uint64_t m2 = tmp;
#else
    state += UINT64_C(0x60bee2bee120fc15);
    __uint128_t tmp;
    tmp = (__uint128_t)state * UINT64_C(0xa3b195354a39b70d);
    uint64_t m1 = (tmp >> 64) ^ tmp;
    tmp = (__uint128_t)m1 * UINT64_C(0x1b03738712fad5c9);
    uint64_t m2 = (tmp >> 64) ^ tmp;
#endif
    return m2;
}

uint64_t GRandomState = 0;

INLINE static float Rand()
{
    auto val = (uint32_t)StatelessNextUint64(GRandomState) >> 10; // 22 bits
    return (float)val / (float)0x3FFFFF;
}

INLINE uint32_t RandUint32()
{
    return (uint32_t)StatelessNextUint64(GRandomState);
}

class GridItemsHelper
{
public:
    struct Item
    {
        uint16_t m_x;
        uint16_t m_y;
        uint16_t m_z;
        uint16_t m_r;
        uint32_t m_data;
        uint32_t m_hash;
        bool m_collected;
        bool m_collectedCheck;
    };

    base::Array<Item> m_items;

    void generate( uint32_t numItems, uint32_t minRadius, uint32_t maxRadius )
    {
        m_items.resize(numItems);

        for (uint32_t i=0; i<numItems; ++i)
        {
            auto& item = m_items[i];
            item.m_data = i;
            item.m_hash = 0;
            item.m_x = (uint16_t)RandUint32();
            item.m_y = (uint16_t)RandUint32();
            item.m_z = (uint16_t)(RandUint32() / 50);
            item.m_r = (uint16_t)(minRadius + Rand() * (maxRadius - minRadius));
        }
    }

    void resetFlags()
    {
        for (auto& it : m_items)
        {
            it.m_collectedCheck = false;
            it.m_collected = false;
        }
    }

    void collectForPoint(int x, int y, int z)
    {
        for (auto& it : m_items)
        {
            int64_t dx = (int64_t)x - (int64_t)it.m_x;
            int64_t dy = (int64_t)y - (int64_t)it.m_y;
            int64_t dz = (int64_t)z - (int64_t)it.m_z;

            int64_t d2 = (dx*dx) + (dy*dy) + (dz*dz);
            int64_t r2 = (int64_t)it.m_r * (int64_t)it.m_r;

            it.m_collectedCheck = (d2 <= r2);           
        }
    }

    void collectForArea(int minX, int minY, int maxX, int maxY)
    {
        for (auto& it : m_items)
        {
            if (it.m_x >= minX && it.m_x <= maxX && it.m_y >= minY && it.m_y <= maxY)
                it.m_collectedCheck = true;
            else
                it.m_collectedCheck = false;
        }
    }
};

namespace helper
{
    static uint32_t CountGridElements(const base::containers::StreamingGrid& grid)
    {
        uint32_t numTotalElems = 0;
        for ( uint32_t i=0; i<NUM_GRID_LEVELS; ++i )
        {
            base::containers::StreamingGridDebugInfo data;
            grid.debugInfo(i, data);
            numTotalElems += data.numElements;
        }
        return numTotalElems;
    }
}

TEST(Streaming, StreamingGrid_10k_AddRemove)
{
    // generate elements
    GridItemsHelper elems;
    elems.generate( 10000, 100, 50000 );

    // create grid
    base::containers::StreamingGrid grid(NUM_GRID_LEVELS, NUM_GRID_BUCKETS);
    EXPECT_EQ(grid.numLevels(), NUM_GRID_LEVELS);
    EXPECT_EQ(grid.maxBuckets(), NUM_GRID_BUCKETS);

    // insert elements into the grid
    for (auto& item : elems.m_items)
    {
        auto hash = grid.registerObject(item.m_x, item.m_y, item.m_z, item.m_r, item.m_data);
        EXPECT_NE(0U, hash);
        item.m_hash = hash;
    }

    // check that elements are added correctly
    EXPECT_EQ(elems.m_items.size(), ::helper::CountGridElements(grid));

    // remove elements from the grid
    for (auto& item : elems.m_items)
        grid.unregisterObject(item.m_hash, item.m_data);

    // expect grid to be empty
    EXPECT_EQ(0, ::helper::CountGridElements(grid));
}

TEST(Streaming, StreamingGrid_10k_AddReverseRemove)
{
    // generate elements
    GridItemsHelper elems;
    elems.generate( 10000, 100, 50000 );

    // create grid
    base::containers::StreamingGrid grid(NUM_GRID_LEVELS, NUM_GRID_BUCKETS);
    EXPECT_EQ(grid.numLevels(), NUM_GRID_LEVELS);
    EXPECT_EQ(grid.maxBuckets(), NUM_GRID_BUCKETS);

    // insert elements into the grid
    for (auto& item : elems.m_items)
    {
        auto hash = grid.registerObject(item.m_x, item.m_y, item.m_z, item.m_r, item.m_data);
        EXPECT_NE(0U, hash);
        item.m_hash = hash;
    }

    // check that elements are added correctly
    EXPECT_EQ(elems.m_items.size(), ::helper::CountGridElements(grid));

    // remove elements from the grid in reverse order
    for (int i= elems.m_items.lastValidIndex(); i >= 0; --i)
    {
        auto& item = elems.m_items[i];
        grid.unregisterObject(item.m_hash, item.m_data);
    }

    // expect grid to be empty
    EXPECT_EQ(0, ::helper::CountGridElements(grid));
}

TEST(Streaming, StreamingGrid_10k_TestPoints)
{
    // generate elements
    GridItemsHelper elems;
    elems.generate(10000, 100, 50000);

    // create grid
    base::containers::StreamingGrid grid(NUM_GRID_LEVELS, NUM_GRID_BUCKETS);
    EXPECT_EQ(grid.numLevels(), NUM_GRID_LEVELS);
    EXPECT_EQ(grid.maxBuckets(), NUM_GRID_BUCKETS);

    // insert elements into the grid
    for (auto& item : elems.m_items)
    {
        auto hash = grid.registerObject(item.m_x, item.m_y, item.m_z, item.m_r, item.m_data);
        EXPECT_NE(0U, hash);
        item.m_hash = hash;
    }

    // check that elements are added correctly
    EXPECT_EQ(elems.m_items.size(), ::helper::CountGridElements(grid));

    // query test
    for (uint32_t i=0; i<500; ++i)
    {
        // test points, radius is different with each test
        uint16_t testPosX = RandUint32();
        uint16_t testPosY = RandUint32();

        // query the grid
        base::containers::StreamingGridCollectorStack<10000> collector;
        grid.collectForPoint(testPosX, testPosY, 0, collector);

        // mark collected item
        elems.resetFlags();
        for (uint32_t i=0; i<collector.size(); ++i)
        {
            auto data = collector[i];
            auto& it = elems.m_items[data];
            EXPECT_EQ(it.m_data, data);
            EXPECT_EQ(it.m_collected, false);
            it.m_collected = true;
        }
                
        // do check
        elems.collectForPoint(testPosX, testPosY, 0);

        // make sure the query results are the same
        for (auto& item : elems.m_items)
        {
            EXPECT_EQ(item.m_collected, item.m_collectedCheck);
            if (item.m_collected != item.m_collectedCheck)
                break;
        }
    }
}

TEST(Streaming, StreamingGrid_10k_TestArea)
{
    // generate elements
    GridItemsHelper elems;
    elems.generate(10000, 100, 50000);

    // create grid
    base::containers::StreamingGrid grid(NUM_GRID_LEVELS, NUM_GRID_BUCKETS);
    EXPECT_EQ(grid.numLevels(), NUM_GRID_LEVELS);
    EXPECT_EQ(grid.maxBuckets(), NUM_GRID_BUCKETS);

    // insert elements into the grid
    for (auto& item : elems.m_items)
    {
        auto hash = grid.registerObject(item.m_x, item.m_y, item.m_z, item.m_r, item.m_data);
        EXPECT_NE(0U, hash);
        item.m_hash = hash;
    }

    // check that elements are added correctly
    EXPECT_EQ(elems.m_items.size(), ::helper::CountGridElements(grid));

    // query test
    for (uint32_t i=0; i<500; ++i)
    {
        // 4 test coordinates
        auto a = (uint16_t)RandUint32();
        auto b = (uint16_t)RandUint32();
        auto c = (uint16_t)RandUint32();
        auto d = (uint16_t)RandUint32();

        // compute test box
        auto minX = std::min(a, b);
        auto maxX = std::min(a, b);
        auto minY = std::min(c, d);
        auto maxY = std::min(c, d);

        // query the grid
        base::containers::StreamingGridCollectorStack<10000> collector;
        grid.collectForArea(minX, minY, maxX, maxY, collector);

        // mark collected item
        elems.resetFlags();
        for (uint32_t i=0; i<collector.size(); ++i)
        {
            uint32_t data = collector[i];
            auto& it = elems.m_items[ data ];
            EXPECT_EQ(it.m_data, data);
            EXPECT_EQ(it.m_collected, false);
            it.m_collected = true;
        }

        // do check
        elems.collectForArea(minX, minY, maxX, maxY);

        // make sure the query results are the same
        for (auto& item : elems.m_items)
        {
            EXPECT_EQ( item.m_collected, item.m_collectedCheck );
            if ( item.m_collected != item.m_collectedCheck )
                break;
        }
    }
}

