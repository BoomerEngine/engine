/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\stats #]
***/

#include "build.h"
#include "poolStats.h"
#include "poolStatsInternal.h"

BEGIN_BOOMER_NAMESPACE_EX(mem)

///--

PoolStats::PoolStats()
{
}

void PoolStats::notifyAllocation(PoolTag id, size_t size)
{
    prv::TheInternalPoolStats.notifyAllocation(id, size);
}

void PoolStats::notifyFree(PoolTag id, size_t size)
{
    prv::TheInternalPoolStats.notifyFree(id, size);
}

void PoolStats::resetGlobalStatistics()
{
    prv::TheInternalPoolStats.resetGlobalStatistics();
}

void PoolStats::resetFrameStatistics()
{
    prv::TheInternalPoolStats.resetFrameStatistics();
}

void PoolStats::stats(PoolTag id, PoolStatsData& outStats) const
{
    prv::TheInternalPoolStats.stats(id, outStats);
}

void PoolStats::allStats(uint32_t count, PoolStatsData* outStats, uint32_t& outNumPools) const
{
    prv::TheInternalPoolStats.allStats(count, outStats, outNumPools);
}

//---

END_BOOMER_NAMESPACE_EX(mem)
