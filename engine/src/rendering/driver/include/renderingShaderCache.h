/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: shaders #]
***/

#pragma once

namespace rendering
{
    //---

    struct ShaderCacheEntryKeyRef
    {
        base::StringView<char> path;
        uint64_t key = 0;

        INLINE ShaderCacheEntryKeyRef()
        {}

        INLINE ShaderCacheEntryKeyRef(base::StringView<char> path_, uint64_t key_)
            : path(path_)
            , key(key_)
        {}

        INLINE bool operator==(const ShaderCacheEntryKeyRef& other) const
        {
            return (key == other.key) && (path == other.path);
        }

        INLINE static uint32_t CalcHash(const ShaderCacheEntryKeyRef& key)
        {
            base::CRC32 crc;
            crc << key.path;
            crc << key.key;
            return crc;
        }
    };

    struct ShaderCacheEntryKey
    {
        base::StringBuf path;
        uint64_t key = 0;

        INLINE ShaderCacheEntryKey()
        {}

        INLINE ShaderCacheEntryKey(const base::StringBuf& path_, uint64_t key_)
            : path(path_)
            , key(key_)
        {}

        INLINE bool operator==(const ShaderCacheEntryKey& other) const
        {
            return (key == other.key) && (path == other.path);
        }

        INLINE bool operator==(const ShaderCacheEntryKeyRef& other) const
        {
            return (key == other.key) && (path == other.path);
        }

        INLINE static uint32_t CalcHash(const ShaderCacheEntryKeyRef& key)
        {
            base::CRC32 crc;
            crc << key.path;
            crc << key.key;
            return crc;
        }

        INLINE static uint32_t CalcHash(const ShaderCacheEntryKey& key)
        {
            base::CRC32 crc;
            crc << key.path;
            crc << key.key;
            return crc;
        }
    };


    //---

    /// shader cache - a collection of shader libraries with metadata on dependencies
    class RENDERING_DRIVER_API ShaderCache : public base::IReferencable
    {
    public:
        ShaderCache();
        ~ShaderCache();

        //---

        /// load shader cache from file
        bool load(const base::io::AbsolutePath& path);

        /// save shader cache to a file (optionally we can only add new entries)
        bool save(const base::io::AbsolutePath& path, bool updatesOnly=false);

        //---

        /// get shader cache entry
        bool fetchEntry(base::StringView<char> path, uint64_t key, ShaderLibraryPtr& outEntry) const;

        /// store shader cache entry
        void storeEntry(base::StringView<char> path, uint64_t key, const ShaderLibraryPtr& entry);

    private:
        base::Mutex m_lock;

        struct InMemoryEntry
        {
            base::StringBuf path;
            uint64_t key = 0;
            uint64_t fileOffset = 0;
            uint64_t fileSize = 0;
            base::Buffer packedData;
            ShaderLibraryPtr unpackedData;
        };

        base::HashMap<ShaderCacheEntryKey, InMemoryEntry*> m_entryMap;
        base::HashSet<InMemoryEntry*> m_newEntries;
    };

    //---

} // rendering
