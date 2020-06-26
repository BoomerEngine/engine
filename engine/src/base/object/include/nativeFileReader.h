/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary\native #]
***/

#include "build.h"
#include "streamBinaryReader.h"

#include "base/io/include/ioFileHandle.h"

namespace base
{
    namespace stream
    {

        /// File reader interface for physical file on disk
        class BASE_OBJECT_API NativeFileReader : public IBinaryReader
        {
        public:
            NativeFileReader(io::IFileHandle& file, uint64_t nativeOffset = 0);
            virtual ~NativeFileReader();

            virtual uint64_t pos() const override final;
            virtual uint64_t size() const override final;
            virtual void seek(uint64_t pos) override final;
            virtual void read(void *data, uint32_t size) override final;

        private:
            static const size_t CACHE_SIZE = 4096;

            void precache(size_t size);

            io::IFileHandle& m_file;

            uint64_t m_size;
            uint64_t m_pos;
            uint64_t m_bufferBase;
            size_t m_bufferCount;

            uint64_t m_nativeOffset;

            uint8_t m_buffer[CACHE_SIZE];
        };

    } // stream

} // base
