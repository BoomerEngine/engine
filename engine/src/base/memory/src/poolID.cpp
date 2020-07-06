/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: allocator\id #]
***/

#include "build.h"
#include "poolID.h"

namespace base
{
    namespace mem
    {
        namespace prv
        {
            // registry of pool names
            class PoolRegistry : public base::NoCopy
            {
            public:
                PoolRegistry()
                    : m_numRegisteredNames(0)
                {
                    // prepare name table
                    memset(&m_names, 0, sizeof(m_names));

                    // setup static pool names
                    m_numRegisteredNames += poolName(POOL_DEFAULT, "Engine.Default");
                    m_numRegisteredNames += poolName(POOL_TEMP, "Engine.Temp");
                    m_numRegisteredNames += poolName(POOL_NEW, "Engine.NewDelete");
                    m_numRegisteredNames += poolName(POOL_PERSISTENT, "Engine.Persistent");
                    m_numRegisteredNames += poolName(POOL_CONTAINERS, "Engine.Containers");
                    m_numRegisteredNames += poolName(POOL_IO, "Engine.IO");
                    m_numRegisteredNames += poolName(POOL_RTTI, "Engine.RTTI");
                    m_numRegisteredNames += poolName(POOL_SCRIPTS, "Engine.Scripts");
                    m_numRegisteredNames += poolName(POOL_NET, "Engine.Network");
                    m_numRegisteredNames += poolName(POOL_IMAGE, "Engine.Images");
                    m_numRegisteredNames += poolName(POOL_STRINGS, "Engine.Strings");
                    m_numRegisteredNames += poolName(POOL_XML, "Engine.XML");
                    m_numRegisteredNames += poolName(POOL_PTR, "Engine.Pointers");
                    m_numRegisteredNames += poolName(POOL_OBJECTS, "Engine.Objects");
                    m_numRegisteredNames += poolName(POOL_RESOURCES, "Engine.Resources");
                    m_numRegisteredNames += poolName(POOL_MEM_BUFFER, "Engine.MemBuffers");
                    m_numRegisteredNames += poolName(POOL_DATA_BUFFER, "Engine.DataBuffers");
                    m_numRegisteredNames += poolName(POOL_ASYNC_BUFFER, "Engine.AsyncBuffers");
                    m_numRegisteredNames += poolName(POOL_SYSTEM_MEMORY, "Engine.SystemMemory");
                    m_numRegisteredNames += 5;
                }

                // get name of the pool
                INLINE const char* name(PoolIDValue value)
                {
                    if (value < m_numRegisteredNames)
                        return m_names[value];
                    return "UNKNOWN";
                }

                // find/register a pool by name
                INLINE PoolIDValue registerPool(const char* name)
                {
                    // fallback to default pool on overflow
                    if (m_numRegisteredNames == MAX_POOLS)
                        return 0;

                    // allocate entry
                    auto newID = range_cast<PoolIDValue>(m_numRegisteredNames++);
                    poolName(newID, name);
                    return newID;
                }

                // register a pool name under ID
                INLINE uint32_t poolName(PoolIDValue id, const char* name)
                {
                    m_names[id] = name;
                    m_hashes[id] = CalcHash(name);

                    auto bucket = m_hashes[id] % MAX_BUCKETS;
                    m_buckets[bucket] = id; // NOTE: this may replace previous value, it's fine for now
                    return 1;
                }

                // get max registered ID
                INLINE PoolIDValue max() const
                {
                    return (PoolIDValue)m_numRegisteredNames;
                }

            private:
                static const uint32_t MAX_POOLS = 256;
                static const uint32_t MAX_BUCKETS = 1024;

                const char* m_names[MAX_POOLS];
                uint32_t m_hashes[MAX_POOLS];
                uint32_t m_numRegisteredNames;

                static INLINE uint32_t CalcHash(const char* ch)
                {
                    uint32_t hval = UINT32_C(0x811c9dc5); // FNV-1a
                    while (*ch)
                    {
                        hval ^= (uint32_t)*ch++;
                        hval *= UINT32_C(0x01000193);
                    }
                    return hval;
                }

                PoolIDValue m_buckets[MAX_BUCKETS];
            };

            static PoolRegistry ThePoolRegistry;
        }

        PoolID::PoolID(const char* name)
        {
            m_value = prv::ThePoolRegistry.registerPool(name);
        }

        const char* PoolID::name() const
        {
            return prv::ThePoolRegistry.name(m_value);
        }

        const char* PoolID::GetPoolNameForID(PoolIDValue id)
        {
            return prv::ThePoolRegistry.name(id);
        }

        PoolIDValue PoolID::GetPoolIDRange()
        {
            return prv::ThePoolRegistry.max();
        }

    } // mem
} // base