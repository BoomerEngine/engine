/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\stats #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base::mem)

/// Pool statistics
struct PoolStatsData
{
    // general stats
    uint32_t m_totalAllocations=0; // total live allocations in the pool
    uint32_t m_maxAllocations = 0; // maximum (since last reset) number of allocation in the pool
    uint64_t m_totalSize = 0; // total size of allocated memory in this pool
    uint64_t m_maxSize = 0; // maximum size of allocated memory in this pool
    uint64_t m_maxAllowedSize = 0; // max allowed size of for this pool (from budgets)

    // frame2frame stats
    uint32_t m_lastFrameAllocs = 0; // number of allocations done since last tracking point (frame)
    uint32_t m_lastFrameFrees = 0; // number of frees done since last tracking point (frame)
    uint64_t m_lastFrameAllocSize = 0; // size of memory allocated since last frame
    uint64_t m_lastFrameFreesSize = 0; // size of memory freed since last frame
};

/// Memory pool statistics tracker
/// NOTE: this is the "get" interface
class BASE_MEMORY_API PoolStats : public ISingleton
{
    DECLARE_SINGLETON(PoolStats);

public:
    PoolStats();

    /// inform about allocation in the pool
    /// NOTE: this is slow frontend interface for big external allocation only
    void notifyAllocation(PoolTag id, size_t size);

    /// inform about deallocation in the pool
    /// NOTE: this is slow frontend interface for big external allocation only
    void notifyFree(PoolTag id, size_t size);

    /// reset global statistics (max allocations & max size)
    void resetGlobalStatistics();

    /// reset frame statistics (local frees&allocs)
    void resetFrameStatistics();

    ///--

    /// get stats for given pool
    void stats(PoolTag id, PoolStatsData& outStats) const;

    /// get stats for all pools
    void allStats(uint32_t count, PoolStatsData* outStats, uint32_t& outNumPools) const;

    ///--
};

END_BOOMER_NAMESPACE(base::mem)