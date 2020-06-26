/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\memory #]
***/

#pragma once

#include "streamBinaryReader.h"

namespace base
{
    namespace stream
    {
        /// a memory based reader
        class BASE_OBJECT_API MemoryReader : public IBinaryReader
        {
        public:
            MemoryReader(const void* data, uint64_t size, uint64_t initialOffset = 0);
            MemoryReader(const Buffer& data, uint64_t initialOffset = 0);
            ~MemoryReader();

            virtual void read(void* data, uint32_t size) override final;
            virtual void read1(void* data) override final;
            virtual void read2(void* data) override final;
            virtual void read4(void* data) override final;
            virtual void read8(void* data) override final;
            virtual void read16(void* data) override final;

            virtual uint64_t pos() const override final;
            virtual uint64_t size() const override final;
            virtual void seek(uint64_t pos) override final;

        private:
            const uint8_t* m_base;
            const uint8_t* m_pos;
            const uint8_t* m_end;
            Buffer m_buffer;
        };
    }
}
