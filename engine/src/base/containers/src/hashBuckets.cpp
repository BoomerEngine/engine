/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: containers #]
***/

#include "build.h"
#include "hashBuckets.h"

namespace base
{
    
    //--

    HashBuckets::HashBuckets()
    {}

    void HashBuckets::Reset(HashBuckets* data)
    {
        if (data)
            memset(data->m_buckets, 0xFF, data->m_bucketCount * sizeof(int));
    }

    void HashBuckets::Clear(HashBuckets*& data)
    {
        MemFree(data);
        data = nullptr;
    }

    bool HashBuckets::CheckCapacity(const HashBuckets* data, uint32_t elementCount)
    {
        if (elementCount < MIN_BUCKETS)
            return true;

        if (data && elementCount < data->m_capacity)
            return true;

        return false;
    }

    //--

} // base
