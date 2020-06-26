/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\memory #]
***/

#pragma once

#include "streamBinaryWriter.h"

namespace base
{
    namespace stream
    {
        /// a memory based writer
        class BASE_OBJECT_API MemoryWriter : public IBinaryWriter
        {
        public:
            MemoryWriter(uint32_t initialReserve = 65535, const mem::PoolID& poolId = POOL_TEMP);
            ~MemoryWriter();

            virtual void write(const void* data, uint32_t size) override final;
            virtual uint64_t pos() const override final;
            virtual uint64_t size() const override final;
            virtual void seek(uint64_t pos) override final;

            // get the buffer
            INLINE const void* data() const { return m_base; }

            // get size of the written data
            INLINE uint32_t dataSize() const { return m_size; }

            // detach the buffer from writer, you now have ownership
            const void* release();

            // reset the writer
            void reset();

            // get memory as buffer (no copy)
            // NOTE: this passes the ownership of the memory to the returned buffer pointer
            Buffer extractData();

        private:
            uint8_t* m_base;
            uint32_t m_capacity;
            uint32_t m_pos;
            uint32_t m_size;
            mem::PoolID m_poolId;
        };
    }
}
