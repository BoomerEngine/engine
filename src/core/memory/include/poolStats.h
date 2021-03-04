/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\stats #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE()

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

//--

// Get readable name of the memory pool
extern CORE_MEMORY_API const char* PoolName(PoolTag tag);

// Get readable name of the memory pool's group (Core, Engine, etc)
extern CORE_MEMORY_API const char*  PoolGroupName(PoolTag tag);

// Get stat's for given memory pool
extern CORE_MEMORY_API void PoolStats(PoolTag tag, PoolStatsData& outData);

// Get all stats
extern CORE_MEMORY_API void PoolStatsAll(PoolStatsData* outStats, uint32_t& numEntries);

//--

/// inform about allocation in the pool
/// NOTE: this is slow frontend interface for big external allocation only
extern CORE_MEMORY_API void PoolNotifyAllocation(PoolTag id, size_t size);

/// inform about deallocation in the pool
/// NOTE: this is slow frontend interface for big external allocation only
extern CORE_MEMORY_API void PoolNotifyFree(PoolTag id, size_t size);

/// reset global statistics (max allocations & max size)
extern CORE_MEMORY_API void PoolResetFrameStatistics();

//--

END_BOOMER_NAMESPACE()
