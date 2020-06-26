/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\stats #]
***/

#include "build.h"
#include "poolID.h"
#include "poolStats.h"
#include "poolStatsInternal.h"

namespace base
{
    namespace mem
    {
        namespace prv
        {
            PoolStatsInternal::PoolStatsInternal()
            {
                // TODO: setup budgets
            }

            void PoolStatsInternal::resetGlobalStatistics()
            {
                auto stat  = m_stats;
                for (uint32_t i=0; i<ARRAY_COUNT(m_stats); ++i, ++stat)
                {
                    stat->m_maxSize = stat->m_totalSize.load();
                    stat->m_maxAllocations = stat->m_totalAllocations.load();
                }
            }

            void PoolStatsInternal::resetFrameStatistics()
            {
                auto stat  = m_stats;
                for (uint32_t i=0; i<ARRAY_COUNT(m_stats); ++i, ++stat)
                {
                    stat->m_lastFrameFreesSize = stat->m_curFrameFreesSize.exchange(0);
                    stat->m_lastFrameFrees = stat->m_curFrameFrees.exchange(0);
                    stat->m_lastFrameAllocs = stat->m_curFrameAllocs.exchange(0);
                    stat->m_lastFrameAllocSize = stat->m_curFrameAllocSize.exchange(0);
                }
            }

            void PoolStatsInternal::stats(PoolID id, PoolStatsData& outStats) const
            {
                auto& src = m_stats[id.value()];

                outStats.m_lastFrameAllocSize = src.m_lastFrameAllocSize;
                outStats.m_lastFrameAllocs = src.m_lastFrameAllocs;
                outStats.m_lastFrameFreesSize = src.m_lastFrameFreesSize;
                outStats.m_lastFrameFrees = src.m_lastFrameFrees;
                outStats.m_totalAllocations = src.m_totalAllocations.load();
                outStats.m_maxAllocations = src.m_maxAllocations.load();
                outStats.m_totalSize = src.m_totalSize.load();
                outStats.m_maxSize = src.m_maxSize.load();

                outStats.m_maxAllowedSize = src.m_maxAllowedSize;

            }

            void PoolStatsInternal::allStats(uint32_t count, PoolStatsData* outStats, uint32_t& outNumPools) const
            {
                // get max pool ID
                auto maxID = PoolID::GetPoolIDRange();
                auto maxToCopy = std::min<uint32_t>(count, maxID);
                for (uint32_t i=0; i<maxToCopy; ++i)
                    stats(PoolID((PredefinedPoolID)i), outStats[i]);

                // clear reset
                auto numLeft = count - maxToCopy;
                memset(outStats + maxToCopy, 0, numLeft * sizeof(PoolStatsData));
                outNumPools = maxToCopy;
            }

            //--

            PoolStatsInternal TheInternalPoolStats;

            //--

        } // prv
    } // mem
} // base