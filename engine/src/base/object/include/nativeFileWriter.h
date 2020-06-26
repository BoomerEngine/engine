/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\native #]
***/

#pragma once

#include "streamBinaryWriter.h"

#include "base/io/include/ioFileHandle.h"

namespace base
{
    namespace stream
    {

        /// File writer for physical file on disk
        class BASE_OBJECT_API NativeFileWriter : public IBinaryWriter
        {
        public:
            //! Constructor
            NativeFileWriter(const io::FileHandlePtr& filePtr);
            virtual ~NativeFileWriter();

            virtual uint64_t pos() const override final;
            virtual uint64_t size() const override final;
            virtual void seek(uint64_t pos) override final;
            virtual void write(const void *data, uint32_t size) override final;

            void flush();

        private:
            static const size_t CACHE_SIZE = 4096;

            io::FileHandlePtr m_file;

            uint8_t m_buffer[CACHE_SIZE];

            uint64_t m_pos;
            size_t m_bufferCount;
        };

    } // stream
} // base


