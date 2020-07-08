/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "resourcePathCache.h"

#include "base/system/include/scopeLock.h"

namespace base
{
    namespace res
    {
        namespace prv
        {

            ResourcePathBuf::ResourcePathBuf(const char* path, size_t len, uint64_t hash)
                : m_hash(hash)
            {
                memcpy(m_txt, path, len);
                m_txt[len] = 0;
            }

            ///

            void ResourcePathCache::deinit()
            {
                ScopeLock<> lock(m_lock);
                m_map.clearPtr();
            }

            uint64_t ResourcePathCache::allocPath(const char* path)
            {
                // empty path case
                if (!path || !path[0])
                    return 0;

                // empty path
                if (0 == strcmp(path, "<none>"))
                    return 0;

                // compute path hash (TODO: standardize)
                auto len = strlen(path);
                uint64_t hash = CRC64().append(path, sizeof(char) * len).crc();

                {
                    ScopeLock<> lock(m_lock);

                    // find in path list
                    ResourcePathBuf* buf = nullptr;
                    if (m_map.find(hash, buf))
                    {
                        DEBUG_CHECK_EX(0 == strcmp(buf->c_str(), path), "Path hash collision");
                        return hash;
                    }

                    // create new entry
                    auto ptr  = MemAlloc(POOL_RESOURCES, sizeof(ResourcePathBuf) + len + 1, 1);
                    buf = new (ptr) ResourcePathBuf(path, len+1, hash);
                    m_map.set(hash, buf);
                    return hash;
                }
            }

            const char* ResourcePathCache::resolvePath(uint64_t hash) const
            {
                // empty path
                if (hash == 0)
                    return "";

                // find
                ScopeLock<> lock(m_lock);

                ResourcePathBuf* buf = nullptr;
                if (m_map.find(hash, buf))
                    return buf->c_str();

                // object not deleted
                return "";
            }

        } // prv

    } // res

} // base