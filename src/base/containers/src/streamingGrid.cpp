/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: streaming #]
***/

#include "build.h"
#include "streamingGrid.h"

BEGIN_BOOMER_NAMESPACE(base)

//--

StreamingGridCollector::StreamingGridCollector(Elem* elements, uint32_t capacity)
    : m_elements(elements)
    , m_numElements(0)
    , m_maxElemenets(capacity)
{}

void StreamingGridCollector::sort()
{
    // sort by the distance key
    // NOTE: the distance key does not have to represent the actual distance, it's more like ordering
    struct
    {
        INLINE bool operator()(const Elem& a, const Elem& b) const
        {
            return a.m_distanceKey < b.m_distanceKey;
        }
    } cmp;

    std::sort(&m_elements[0], &m_elements[m_numElements], cmp);
}

//----

StreamingGrid::StreamingGrid(uint32_t numLevels, uint32_t numBuckets)
{
    // initialize the buckets
    createBuckets(numBuckets);

    // initialize the grid
    createGrid(numLevels);
}

StreamingGrid::~StreamingGrid()
{
    if (m_buckets)
    {
        mem::GlobalPool<POOL_STREAMING, GridBucket>::Free(m_buckets);
        m_buckets = nullptr;
    }

    if (m_nodes)
    {
        mem::GlobalPool<POOL_STREAMING, GridNode>::Free(m_nodes);
        m_nodes = nullptr;
    }

    if (m_levels)
    {
        mem::GlobalPool<POOL_STREAMING, GridLevel>::Free(m_levels);
        m_levels = nullptr;
    }
}

void StreamingGrid::debugInfo(uint32_t level, struct StreamingGridDebugInfo& outData) const
{
    if (level >= m_numLevels)
    {
        outData = StreamingGridDebugInfo();
        return;
    }

    // process grid at given level
    const GridLevel& l = m_levels[level];
    outData.numCells = l.m_nodeCount;

    // grid counts
    outData.m_gridSize = l.m_levelSize;
    outData.m_gridElemCount.resize(l.m_levelSize*l.m_levelSize);
    outData.m_gridMaxElemCount = 0;

    // count buckets
    for (uint32_t y = 0; y < l.m_levelSize; ++y)
    {
        for (uint32_t x = 0; x < l.m_levelSize; ++x)
        {
            const GridNode& n = l.m_nodes[x + y*l.m_levelSize];

            uint32_t numCellElements = 0;
            TBucketIndex bucketIndex = n.m_bucket;
            while (bucketIndex != 0)
            {
                const GridBucket& b = m_buckets[bucketIndex];

                // count elements
                outData.numBuckets += 1;
                outData.numElements += b.m_elemCount;
                numCellElements += b.m_elemCount;

                // extract elements
                for (uint32_t j = 0; j < b.m_elemCount; ++j)
                {
                    const GridElement& e = b.m_elems[j];

                    // emit debug element
                    auto& elemInfo = outData.m_elems.emplaceBack();
                    elemInfo.x = (float)e.m_x / 65535.0f;
                    elemInfo.y = (float)e.m_y / 65535.0f;
                    elemInfo.r = (float)e.m_radius / 65535.0f;
                }

                // next bucket
                bucketIndex = b.m_nextBucket;
            }

            // count wasted elements
            if (n.m_bucketCount > 1)
            {
                outData.m_numWastedElements += (n.m_bucketCount*GridBucket::MAX - numCellElements);
            }

            // update histogram
            outData.m_gridElemCount[x + y*l.m_levelSize] = numCellElements;
            if (numCellElements > outData.m_gridMaxElemCount)
                outData.m_gridMaxElemCount = numCellElements;
        }
    }
}

StreamingGridObjectID StreamingGrid::registerObject(uint16_t x, uint16_t y, uint16_t z, uint16_t radius, uint32_t data)
{
    // calculate on which level we should put the entry
    int level = m_numLevels - 1;
    while (level > 0)
    {
        int levelShift = 15 - level;
        int levelRadius = 1 << levelShift; // level0 grid has 2x2 cells

        if (radius < levelRadius)
            break;

        --level;
    }

    // calculate cell index in the entry
    int levelShift = 15 - level;
    int levelCount = 2 << level;
    uint32_t cellIndex = (x >> levelShift) + (y >> levelShift) * levelCount;

    // validation
    ASSERT_EX(level < (int)m_numLevels, "Invalid quantization result");
    ASSERT_EX(cellIndex < m_levels[level].m_nodeCount, "Invalid quantization result");

    // add to cell, try to add to an existing bucket first
    auto node  = m_levels[level].m_nodes + cellIndex;
    auto bucketIndex = node->m_bucket;
    while (bucketIndex != 0)
    {
        auto b  = &m_buckets[bucketIndex];
        if (b->m_elemCount < GridBucket::MAX)
        {
            // allocate element in bucket
            auto e  = &b->m_elems[b->m_elemCount];
            b->m_elemCount += 1;

            // store element values
            e->m_x = x;
            e->m_y = y;
            e->m_z = z;
            e->m_radius = radius;
            e->m_data = data;

#ifdef DEBUG_STREAMING_GRID
            TRACE_INFO("Added 0x{} at bucket {} pos [{},{},{}], radius {}, level {}, elem {}"),
                data, bucketIndex, x, y, z, radius, level, b->m_elemCount);
#endif

                // return object ID - bucket index
                return bucketIndex;
        }

        // try in next node
        bucketIndex = b->m_nextBucket;
    }

    // no free space, create new bucket
    bucketIndex = 0;// m_bucketIDs.allocate();
    if (bucketIndex == m_numBuckets)
    {
        TRACE_ERROR("Initial bucket array was to small, resizing the bucket table");
        m_numBuckets *= 2;
        m_buckets = mem::GlobalPool<POOL_STREAMING, GridBucket>::Resize(m_buckets, sizeof(GridBucket) * m_numBuckets, 64);
    }

    // Store in new bucket
    auto b  = &m_buckets[bucketIndex];
    auto e  = &b->m_elems[0];

    // store element values
    e->m_x = x;
    e->m_y = y;
    e->m_z = z;
    e->m_radius = radius;
    e->m_data = data;

    // update node
    b->m_elemCount = 1;
    b->m_nextBucket = node->m_bucket;
    node->m_bucket = bucketIndex;
    node->m_bucketCount += 1;

#ifdef DEBUG_STREAMING_GRID
    TRACE_INFO("Added 0x{} at bucket {} pos [{},{},{}], radius {}, level {}, elem {}"),
        Hex(data), bucketIndex, x, y, z, radius, level, b->m_elemCount );
#endif

        // bucket index is the grid hash
        return bucketIndex;
}

void StreamingGrid::unregisterObject(StreamingGridObjectID hash, uint32_t data)
{
    ASSERT_EX(hash != 0, "Trying to unregister invalid object from the grid");
    ASSERT_EX(hash < m_numBuckets, "Trying to unregister invalid object from the grid");

    // find the object in the bucket
    auto& b = m_buckets[hash];
    ASSERT_EX(b.m_elemCount > 0, "Hash pointing to empty bucket");
    for (uint32_t i = 0; i < b.m_elemCount; ++i)
    {
        if (b.m_elems[i].m_data == data)
        {
            // cleanup the last element
            b.m_elemCount -= 1;

#ifdef DEBUG_STREAMING_GRID
            auto& e = b.m_elems[i];
            TRACE_INFO("Removed 0x{} at bucket {} pos [{},{},{}], rad {}, elem {}/{}"),
                Hex(data), hash, e.x, e.m_y, e.m_z, e.m_radius, i, b.m_elemCount );
#endif

                // shift
                if (i != b.m_elemCount)
                {
                    b.m_elems[i] = b.m_elems[b.m_elemCount];
                }

                // cleanup last element
                b.m_elems[b.m_elemCount].m_data = 0;
                b.m_elems[b.m_elemCount].m_x = 0;
                b.m_elems[b.m_elemCount].m_y = 0;
                b.m_elems[b.m_elemCount].m_z = 0;
                b.m_elems[b.m_elemCount].m_radius = 0;

                // last element removed from bucket ?
                if (b.m_elemCount == 0)
                {
                    // TODO: find the related grid node
                    // TODO: unlink the bucked from the node linked list
                    // TODO: release the bucket to the pool

                }

                // deleted
                return;
        }
    }

    // value not found in bucket
    FATAL_ERROR("Object not found in the streaming grid");
}

void StreamingGrid::unregisterObject(uint16_t x, uint16_t y, uint16_t z, uint16_t radius, uint32_t data)
{
    // calculate on which level we should put the entry
    int level = m_numLevels - 1;
    while (level > 0)
    {
        int levelShift = 15 - level;
        int levelRadius = 1 << levelShift; // level0 grid has 2x2 cells

        if (radius < levelRadius)
            break;

        --level;
    }

    // calculate cell index in the entry
    int levelShift = 15 - level;
    int levelCount = 2 << level;
    uint32_t cellIndex = (x >> levelShift) + (y >> levelShift) * levelCount;

    // validation
    ASSERT_EX(level < (int)m_numLevels, "Invalid quantization result");
    ASSERT_EX(cellIndex < m_levels[level].m_nodeCount, "Invalid quantization result");

    // add to cell, try to add to an existing bucket first
    auto node  = m_levels[level].m_nodes + cellIndex;
    auto bucketIndex = node->m_bucket;
    while (bucketIndex != 0)
    {
        auto& b = m_buckets[bucketIndex];
        for (uint32_t i = 0; i < b.m_elemCount; ++i)
        {
            auto& e = b.m_elems[i];
            if (e.m_data == data)
            {
                // cleanup the last element
                b.m_elemCount -= 1;

#ifdef DEBUG_STREAMING_GRID
                auto& e = b.m_elems[i];
                TRACE_INFO("Removed 0x{} at bucket {} pos [{},{},{}], radius {}, elem {}/{}"),
                    Hex(data), hash, e.x, e.m_y, e.m_z, e.m_radius, i, b.m_elemCount );
#endif

                    // shift
                    if (i != b.m_elemCount)
                    {
                        b.m_elems[i] = b.m_elems[b.m_elemCount];
                    }

                    // cleanup last element
                    b.m_elems[b.m_elemCount].m_data = 0;
                    b.m_elems[b.m_elemCount].m_x = 0;
                    b.m_elems[b.m_elemCount].m_y = 0;
                    b.m_elems[b.m_elemCount].m_z = 0;
                    b.m_elems[b.m_elemCount].m_radius = 0;

                    // last element removed from bucket ?
                    if (b.m_elemCount == 0)
                    {
                        // TODO: find the related grid node
                        // TODO: unlink the bucked from the node linked list
                        // TODO: release the bucket to the pool
                        // m_bucketIDs.Release( hash );
                    }

                    // deleted
                    return;
            }
        }

        // try in next bucket
        bucketIndex = b.m_nextBucket;
    }

    // value not found in bucket
    FATAL_ERROR("Object not found in the streaming grid");
}

StreamingGridObjectID StreamingGrid::moveObject(StreamingGridObjectID hash, uint16_t x, uint16_t y, uint16_t z, uint32_t data)
{
    ASSERT_EX(hash != 0, "Trying to move entry not registered in the grid");

    // find existing element
    GridElement* existingData = nullptr;
    if (hash != 0)
    {
        ASSERT_EX(hash < m_numBuckets, "Trying to unregister invalid object from the grid");

        // find the object in the bucket
        auto& b = m_buckets[hash];
        for (uint32_t i = 0; i < b.m_elemCount; ++i)
        {
            auto& e = b.m_elems[i];
            if (e.m_data == data)
            {
                existingData = &e;
                break;
            }
        }
    }

    // try to reuse existing element (only allowed if radius is the same)
    if (existingData == nullptr)
    {
        FATAL_ERROR("Object not found in the streaming grid");
        return 0;
    }

    // calculate on which level we should put the entry
    int level = m_numLevels - 1;
    auto radius = existingData->m_radius;
    while (level > 0)
    {
        int levelShift = 15 - level;
        int levelRadius = 1 << levelShift; // level0 grid has 2x2 cells

        if (radius < levelRadius)
            break;

        --level;
    }

    // are we in the same grid cell ?
    int levelShift = 15 - level;
    uint32_t oldCX = (existingData->m_x) >> levelShift;
    uint32_t oldCY = (existingData->m_y) >> levelShift;
    uint32_t newCX = (x) >> levelShift;
    uint32_t newCY = (y) >> levelShift;
    if (oldCX == newCX && oldCY == newCY)
    {
#ifdef DEBUG_STREAMING_GRID
        auto& e = *existingData;
        TRACE_INFO("Updated 0x{} at bucket {} pos [{},{},{}], rad {}"),
            Hex(data), hash, e.x, e.m_y, e.m_z, e.m_radius );
#endif

            // in the same cell, just update the location
            existingData->m_x = x;
            existingData->m_y = y;
            existingData->m_z = z;
            return hash;
    }

    // not in the same cell, cleanup 
    unregisterObject(hash, data);

    // register in new place
    return registerObject(x, y, z, radius, data);
}

void StreamingGrid::collectAll(class StreamingGridCollector& outCollector) const
{
    for (uint32_t i=0; i<m_numBuckets; ++i)
    {
        auto& b = m_buckets[i];
        for (uint32_t j=0; j<b.m_elemCount; ++j)
            outCollector.add(b.m_elems[j].m_data, 0.0f);
    }
}

void StreamingGrid::collectForPoint(uint16_t testX, uint16_t testY, uint16_t testZ, StreamingGridCollector& outCollector) const
{
    PC_SCOPE_LVL1(StreamingGridCollectPoint);

    // visit each level, start from the bottom
    int maxLevel = (m_numLevels - 1);
    int cellX = testX >> (15 - maxLevel);
    int cellY = testY >> (15 - maxLevel);
    for (int level = maxLevel; level >= 0; --level)
    {
        auto nodes  = m_levels[level].m_nodes;

        // number of cells in this level
        int levelSize = 2 << level;
        ASSERT_EX(cellY < levelSize && cellX < levelSize, "Cell coordinate out of range");
                
        // visit cell and touching cells (3x3 grid)
        // we know from the construction of the grid that we don't have to check any further
        int minX = std::max<int>(0, cellX - 1);
        int minY = std::max<int>(0, cellY - 1);
        int maxX = std::min<int>(cellX + 1, levelSize - 1);
        int maxY = std::min<int>(cellY + 1, levelSize - 1);
        ASSERT_EX(maxX - minX <= 3, "To many nodes to check in level");
        ASSERT_EX(maxY - minY <= 3, "To many nodes to check in level");

        // debug stuff
#ifdef DEBUG_STREAMING_GRID
        TRACE_INFO("Level[{}]: [{},{}]-[{},{}]"), level, minX, minY, maxX, maxY );
#endif

        // scan buckets in each node
        for (int y = minY; y <= maxY; ++y)
        {
            for (int x = minX; x <= maxX; ++x)
            {
                int cellIndex = x + (y*levelSize);

                auto bucketIndex = nodes[cellIndex].m_bucket;
                while (bucketIndex != 0)
                {
                    auto& bucket = m_buckets[bucketIndex];

                    // debug stuff
#ifdef DEBUG_STREAMING_GRID
                    TRACE_INFO("  Cell[{},{}]: {}, bucket {}, elems {}"), x, y, cellIndex, bucketIndex, bucket.m_elemCount );
#endif

                    // test elements, all elements from this bucket are in cache, dirt cheap test
                    for (uint32_t c = 0; c < bucket.m_elemCount; ++c)
                    {
                        auto& e = bucket.m_elems[c];

                        // calculate squared distance
                        int64_t dx = (int64_t)e.m_x - (int64_t)testX;
                        int64_t dy = (int64_t)e.m_y - (int64_t)testY;
                        int64_t dz = (int64_t)e.m_z - (int64_t)testZ;
                        int64_t d2 = (dx*dx) + (dy*dy) + (dz*dz);

                        // validate distance - should not be bigger than two times the cell size
                        int cellSize = 2 * ((1 << 15) >> level);
                        ASSERT_EX(abs(dx) <= cellSize && abs(dy) <= cellSize, "Distance is larger than cell size");

                        // check
                        int64_t r2 = (int64_t)e.m_radius * (int64_t)e.m_radius;
                        if (d2 <= r2)
                            outCollector.add(e.m_data, d2);
                    }

                    // advance to next bucket
                    bucketIndex = bucket.m_nextBucket;
                }
            }
        }

        // adjust cell pos for the upper level
        cellX /= 2;
        cellY /= 2;
    }

    // we should end up in 0,0 cell
    ASSERT_EX(cellX == 0, "Cell counting problem");
    ASSERT_EX(cellY == 0, "Cell counting problem");
}

void StreamingGrid::collectForArea(uint16_t minX, uint16_t minY, uint16_t maxX, uint16_t maxY, class StreamingGridCollector& outCollector) const
{
    PC_SCOPE_LVL1(StreamingGridCollectArea);

    // visit each level, start from the bottom
    int maxLevel = (m_numLevels - 1);
    int cellMinX = minX >> (15 - maxLevel);
    int cellMinY = minY >> (15 - maxLevel);
    int cellMaxX = maxX >> (15 - maxLevel);
    int cellMaxY = maxY >> (15 - maxLevel);
    for (int level = maxLevel; level >= 0; --level)
    {
        auto nodes  = m_levels[level].m_nodes;

        // number of cells in this level
        int levelSize = 2 << level;
        ASSERT_EX(cellMinX < levelSize && cellMinY < levelSize && cellMaxX < levelSize && cellMaxY < levelSize, "Cell coordinate out of range");

        // scan buckets in each node
        for (int y = cellMinY; y <= cellMaxY; ++y)
        {
            for (int x = cellMinX; x <= cellMaxX; ++x)
            {
                int cellIndex = x + (y*levelSize);

                auto bucketIndex = nodes[cellIndex].m_bucket;
                while (bucketIndex != 0)
                {
                    auto& bucket = m_buckets[bucketIndex];

                    // test elements, all elements from this bucket are in cache, dirt cheap test
                    for (uint32_t c = 0; c < bucket.m_elemCount; ++c)
                    {
                        auto& e = bucket.m_elems[c];

                        // test proxy position
                        if (e.m_x >= minX && e.m_x <= maxX && e.m_y >= minY && e.m_y <= maxY)
                            outCollector.add(e.m_data, 0);
                    }

                    // advance to next bucket
                    bucketIndex = bucket.m_nextBucket;
                }
            }
        }

        // adjust cell pos for the upper level
        cellMinX /= 2;
        cellMinY /= 2;
        cellMaxX /= 2;
        cellMaxY /= 2;
    }

    // we should end up in 0,0 cell
    ASSERT_EX(cellMinX == 0 && cellMinY == 0 && cellMaxX == 0 && cellMaxY == 0, "Cell counting problem");
}

void StreamingGrid::createBuckets(uint32_t numBuckets)
{
    m_numBuckets = numBuckets;
    m_buckets = base::mem::GlobalPool<POOL_STREAMING, GridBucket>::AllocN(numBuckets);
    memzero(m_buckets, numBuckets * sizeof(GridBucket));
}

void StreamingGrid::createGrid(uint32_t numLevels)
{
    // calculate the total cell count
    uint32_t numTotalCells = 0;
    for (uint32_t i = 0; i < numLevels; ++i)
    {
        uint32_t gridSize = 2 << i; // level0 - 2x2, level1- 4x4, etc
        numTotalCells += gridSize * gridSize;
    }

    // preallocate memory for nodes
    m_numNodes = numTotalCells;
    m_nodes = mem::GlobalPool<POOL_STREAMING, GridNode>::AllocN(m_numNodes);
    memzero(m_nodes, sizeof(GridNode) * m_numNodes);

    // preallocate memory for grid levels
    m_numLevels = numLevels;
    m_levels = mem::GlobalPool<POOL_STREAMING, GridLevel>::AllocN(m_numLevels);

    // setup levels
    numTotalCells = 0;
    for (uint32_t i = 0; i < numLevels; ++i)
    {
        uint32_t gridSize = 2 << i; // level0 - 2x2, level1- 4x4, etc

        m_levels[i].m_levelSize = (uint16_t)gridSize;
        m_levels[i].m_nodeCount = gridSize * gridSize;
        m_levels[i].m_nodes = &m_nodes[numTotalCells];

        numTotalCells += gridSize * gridSize;
    }

    // stats
    TRACE_INFO("Create streaming grid with {} levels, {} cells ({})", numLevels, numTotalCells, TimeInterval(sizeof(GridNode) * m_numNodes));
}

END_BOOMER_NAMESPACE(base)
