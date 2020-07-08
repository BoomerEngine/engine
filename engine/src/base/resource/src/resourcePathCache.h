/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "base/system/include/atomic.h"
#include "base/system/include/mutex.h"
#include "base/containers/include/hashMap.h"
#include "base/containers/include/stringBuf.h"

namespace base
{
    namespace res
    {
        namespace prv
        {

            class ResourcePathCache;

            /// actual holder for path to the resource
            /// this object is unique and exists only once in the memory engine
            /// Still in final game should not be needed
            class ResourcePathBuf
            {
            public:
                ResourcePathBuf(const char* path, size_t len, uint64_t hash);

                INLINE const char* c_str() const
                {
                    return m_txt;
                }

                INLINE uint64_t hash() const
                {
                    return m_hash;
                }

            private:
                uint64_t m_hash;
                char m_txt[1];
            };

            /// Cache for resource paths - every path is defined only once
            class ResourcePathCache : public ISingleton
            {
                DECLARE_SINGLETON(ResourcePathCache);

            public:
                /// allocate/find path entry for given string
                uint64_t allocPath(const char* path);

                /// resolve string for given path hash
                const char* resolvePath(uint64_t hash) const;

            private:
                typedef HashMap<uint64_t, ResourcePathBuf*> TPathMap;
                typedef Mutex TPathMapLock;

                TPathMap m_map;
                TPathMapLock m_lock;

                virtual void deinit() override;
            };

        } // prv
    } // res
} // base