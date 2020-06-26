/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\id #]
***/

#pragma once

#include "base/system/include/mutex.h"

namespace base
{
    namespace mem
    {
        typedef uint8_t PoolIDValue;

        /// ID of the memory pool
        /// In general it's just a compile time string but we can query the memory system for all of the PoolIDs for stats
        /// Since we store the ID as byte there's a limit of 256 pools in the system, ID 0 is the generic pool
        /// Many allocators take the pool ID as specifier
        /// The PoolID can be structural, ie. "Engine.IO" to provide basic hierarchy for the statistics
        class BASE_MEMORY_API PoolID
        {
        public:
            // create the default pool ID (the default pool, a.k.a "The Bucket")
            INLINE PoolID()
                : m_value(0)
            {}

            // create the pool ID from the predefined enum
            INLINE PoolID(PredefinedPoolID id)
                : m_value((uint8_t)id)
            {}

            // copy
            INLINE PoolID(const PoolID& other)
                    : m_value(other.m_value)
            {}

            // create named pool ID
            // NOTE: same name gets the same internal ID
            // NOTE: we only have 256 entries possible, use with care
            PoolID(const char* name);

            //--

            // get name of this pool
            const char* name() const;

            // get numerical value for the ID of this pool
            INLINE PoolIDValue value() const { return m_value; }

            //--

            // get name for PoolID, returns null if the pool is not registered
            static const char* GetPoolNameForID(PoolIDValue id);

            // get the number of PoolIDs registered
            static PoolIDValue GetPoolIDRange();

        private:
            PoolIDValue m_value;
        };

    } // mem
} // base