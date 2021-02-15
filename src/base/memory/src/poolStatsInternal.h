/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\stats #]
***/

#include "build.h"
#include "poolStats.h"

#include "base/system/include/atomic.h"

namespace base
{
    namespace mem
    {
        namespace prv
        {

        /// pool stats tracking (internal version)
        /// NOTE: this class is inlined for best performance
        class PoolStatsInternal
        {
        public:
            PoolStatsInternal();

            INLINE void notifyAllocation(PoolTag id, size_t size)
            {
#ifndef BUILD_FINAL
                auto& stats = m_stats[id];
                auto curAlloc = ++stats.m_totalAllocations;
                AtomicMax(stats.m_maxAllocations, curAlloc);
                auto curSize = stats.m_totalSize += size;
                AtomicMax(stats.m_maxSize, curSize);
                ++stats.m_curFrameAllocs;
                stats.m_curFrameAllocSize += size;
#endif
            }

            INLINE void notifyFree(PoolTag id, size_t size)
            {
#ifndef BUILD_FINAL
                auto& stats = m_stats[id];
                --stats.m_totalAllocations;
                stats.m_totalSize -= size;
                stats.m_maxSize -= size;
                ++stats.m_curFrameFrees;
                stats.m_curFrameFreesSize += size;
#endif
            }

            ///--

            /// reset global statistics (max allocations & max size)
            void resetGlobalStatistics();

            /// reset frame statistics (local frees&allocs)
            void resetFrameStatistics();

            /// get stats for given pool
            void stats(PoolTag id, PoolStatsData& outStats) const;

            /// get stats for all pools
            void allStats(uint32_t count, PoolStatsData* outStats, uint32_t& outNumPools) const;

        private:
#ifdef PLATFORM_64BIT
            typedef std::atomic<uint64_t> SizeAtomic;
            typedef std::atomic<uint32_t> CountAtomic;
#else
            typedef std::atomic<uint32_t> SizeAtomic;
            typedef std::atomic<uint32_t> CountAtomic;
#endif

            struct Stats
            {
                CountAtomic m_totalAllocations = 0; // total live allocations in the pool
                CountAtomic m_maxAllocations = 0; // maximum (since last reset) number of allocation in the pool
                SizeAtomic m_totalSize = 0; // total size of allocated memory in this pool
                SizeAtomic m_maxSize = 0; // maximum size of allocated memory in this pool
                CountAtomic m_curFrameAllocs = 0; // number of allocations done since last tracking point (frame)
                CountAtomic m_curFrameFrees = 0; // number of frees done since last tracking point (frame)
                SizeAtomic m_curFrameAllocSize = 0; // size of memory allocated since last frame
                SizeAtomic m_curFrameFreesSize = 0; // size of memory freed since last frame

                uint32_t m_lastFrameAllocs = 0; // number of allocations done since last tracking point (frame)
                uint32_t m_lastFrameFrees = 0; // number of frees done since last tracking point (frame)
                uint64_t m_lastFrameAllocSize = 0; // size of memory allocated since last frame
                uint64_t m_lastFrameFreesSize = 0; // size of memory freed since last frame
                uint64_t m_maxAllowedSize = 0; // max allowed size of for this pool (from budgets)
            };

            Stats m_stats[POOL_MAX];
        };

        ///--

        extern PoolStatsInternal TheInternalPoolStats;

        ///--

        } // prv
    } // mem
} // base