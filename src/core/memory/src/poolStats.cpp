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

BEGIN_BOOMER_NAMESPACE()

///--

const char* PoolName(PoolTag tag)
{
    switch (tag)
    {
#define BOOMER_DECLARE_POOL(name, group, size) case name: return #name + 5;
#include "poolNames.inl"
    }

    return "UNKNOWN";
}

const char* PoolGroupName(PoolTag tag)
{
    switch (tag)
    {
#define BOOMER_DECLARE_POOL(name, group, size) case name: return group;
#include "poolNames.inl"
    }

    return "UNKNOWN";
}

void PoolStats(PoolTag tag, PoolStatsData& outData)
{
    prv::TheInternalPoolStats.stats(tag, outData);

}

void PoolStatsAll(PoolStatsData* outStats, uint32_t& numEntries)
{
    prv::TheInternalPoolStats.allStats(numEntries, outStats, numEntries);
}

void PoolNotifyAllocation(PoolTag id, size_t size)
{
    prv::TheInternalPoolStats.notifyAllocation(id, size);
}

void PoolNotifyFree(PoolTag id, size_t size)
{
    prv::TheInternalPoolStats.notifyFree(id, size);
}

void PoolResetFrameStatistics()
{
    prv::TheInternalPoolStats.resetFrameStatistics();
}

//---

END_BOOMER_NAMESPACE()
