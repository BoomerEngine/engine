/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
*/

#include "build.h"
#include "renderingShaderLibrary.h"
#include "renderingShaderCache.h"

namespace rendering
{
    //--

    ShaderCache::ShaderCache()
    {}

    ShaderCache::~ShaderCache()
    {
        m_entryMap.clearPtr();
    }

    bool ShaderCache::load(base::StringView<char> path)
    {
        // TODO
        return false;
    }

    bool ShaderCache::save(base::StringView<char> path, bool updatesOnly /*= false*/)
    {
        return true;
    }

    bool ShaderCache::fetchEntry(base::StringView<char> path, uint64_t key, ShaderLibraryPtr& outEntry) const
    {
        auto lock = CreateLock(m_lock);

        ShaderCacheEntryKeyRef localKey(path, key);
        InMemoryEntry* entry = nullptr;
        if (m_entryMap.find(localKey, entry))
        {
            if (!entry->unpackedData)
            {
                /*entry->unpackedData = base::rtti_cast<ShaderLibrary>(base::res::LoadUncached("", ShaderLibrary::GetStaticClass(), entry->packedData.data(), entry->packedData.size()));
                if (!entry->unpackedData)
                {
                    TRACE_ERROR("Unable to unpack cached shader entry '{}' key {}", path, Hex(key));
                    return false;
                }*/
            }

            outEntry = entry->unpackedData;
            return true;
        }

        return false;
    }

    void ShaderCache::storeEntry(base::StringView<char> path, uint64_t key, const ShaderLibraryPtr& data)
    {
        auto lock = CreateLock(m_lock);

        ShaderCacheEntryKeyRef localKey(path, key);
        InMemoryEntry* entry = nullptr;
        if (m_entryMap.find(localKey, entry))
        {
            entry->packedData.reset();
            entry->unpackedData = data;
            m_newEntries.insert(entry);
        }
        else
        {
            entry = MemNew(InMemoryEntry);
            entry->path = base::StringBuf(path);
            entry->key = key;
            entry->unpackedData = data;

            ShaderCacheEntryKey localKeyPerm(entry->path, key);
            m_entryMap[localKeyPerm] = entry;
            m_newEntries.insert(entry);
        }
    }

    //--

} // rendering
