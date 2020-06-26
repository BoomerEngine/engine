/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: io\system #]
***/

#pragma once

#include "ioFileHandle.h"

namespace base
{
    namespace io
    {
        //--

        // an simple wrapper for file handle interface that allows read from memory block
        class BASE_IO_API MemoryReaderFileHandle : public IFileHandle
        {
        public:
            MemoryReaderFileHandle(const void* memory, uint64_t size, const StringBuf& origin = StringBuf::EMPTY());
            MemoryReaderFileHandle(const Buffer& buffer, const StringBuf& origin = StringBuf::EMPTY());
            virtual ~MemoryReaderFileHandle();

            //----

            /// IFileHandler
            virtual const StringBuf& originInfo() const override final;
            virtual uint64_t size() const override final;
            virtual uint64_t pos() const override final;
            virtual bool pos(uint64_t newPosition) override final;

            virtual bool isReadingAllowed() const override final;
            virtual bool isWritingAllowed() const override final;

            virtual uint64_t writeSync(const void* data, uint64_t size) override final;
            virtual uint64_t readSync(void* data, uint64_t size) override final;

            virtual CAN_YIELD uint64_t readAsync(uint64_t offset, uint64_t size, void* readBuffer) override final;

        protected:
            uint64_t m_pos;
            uint64_t m_size;
            const uint8_t* m_data;
            Buffer m_buffer;
            StringBuf m_origin;
        };

    } // io
} // base
