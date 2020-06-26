/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\memory #]
***/

#include "build.h"
#include "memoryWriter.h"

namespace base
{
    namespace stream
    {
        MemoryWriter::MemoryWriter(const uint32_t initialReserve /*= 65535*/, const mem::PoolID& poolId /*= POOL_TEMP*/)
            : IBinaryWriter((uint32_t)BinaryStreamFlags::MemoryBased)
            , m_capacity(initialReserve)
            , m_base(nullptr)
            , m_poolId(poolId)
            , m_pos(0)
            , m_size(0)
        {
            if (initialReserve)
            {
                auto ptr  = (uint8_t*)mem::AAllocSystemMemory(initialReserve, false);
                if (ptr == nullptr)
                {
                    reportError("Out of memory");
                }
                else
                {
                    m_base = ptr;
                    m_capacity = initialReserve;
                }
            }
        }

        MemoryWriter::~MemoryWriter()
        {
            if (m_base != nullptr)
            {
                mem::AFreeSystemMemory(m_base, m_capacity);
                m_base = nullptr;
            }
        }

        void MemoryWriter::reset()
        {
            m_size = 0;
            m_pos = 0;
        }

        void MemoryWriter::write(const void* data, uint32_t size)
        {
            // trying to write to a buffer with errors makes no sense
            if (isError())
                return;

            // resize if needed
            while (m_pos + size > m_capacity)
            {
                // calculate new capacity 
                auto oldCapacity = m_capacity;
                auto newCapacity = std::max<uint32_t>(65536, oldCapacity * 2);

                // allocate new memory buffer
                auto ptr  = (uint8_t*)mem::AAllocSystemMemory(newCapacity, false);
                if (ptr == nullptr)
                {
                    reportError("Out of memory");
                    return;
                }
                else
                {
                    memcpy(ptr, m_base, m_size);
                    mem::AFreeSystemMemory(m_base, oldCapacity);
                    m_capacity = newCapacity;
                    m_base = ptr;
                }
            }

            // write data
            memcpy(m_base + m_pos, data, size);

            // advance write pointer
            m_pos += size;
            m_size = std::max<uint32_t>(m_size, m_pos);
        }

        uint64_t MemoryWriter::pos() const
        {
            return m_pos;
        }

        uint64_t MemoryWriter::size() const
        {
            return m_size;
        }

        void MemoryWriter::seek(uint64_t pos)
        {
            if (pos >= UINT32_MAX)
                reportError("IO: Trying to seek past the maximum allowed memory buffer size");
            else
                m_pos = range_cast<uint32_t>(pos);
        }

        const void* MemoryWriter::release()
        {
            auto ret  = m_base;
            m_base = nullptr;
            return ret;
        }

        static void SystemMemoryFreeFunc(mem::PoolID pool, void* memory, uint64_t size)
        {
            mem::AFreeSystemMemory(memory, size);
        }

        Buffer MemoryWriter::extractData()
        {
            // prevent extracting data from corrupted memory writer
            if (isError())
            {
                TRACE_ERROR("Trying to extract content from corrupted memory writer.");
                return nullptr;
            }

            // no data to extract
            if (!m_size)
                return Buffer();

            // create output buffer
            Buffer ret = Buffer::CreateExternal(POOL_TEMP, m_size, m_base, &SystemMemoryFreeFunc);

            // reset buffer
            m_base = nullptr;
            m_capacity = 0;
            m_pos = 0;
            m_size = 0;
            return ret;
        }


    } // stream
} // base

