/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"
#include "streamBinaryFlags.h"

namespace base
{
    namespace stream
    {

        //---

        class IBinaryReader;
        class IBinaryWriter;

        /// A base file like object
        class BASE_OBJECT_API IStream
        {
        public:
            uint32_t m_flags;
            uint32_t m_version;

            IStream(uint32_t flags);
            virtual ~IStream();

            //! Get file position
            virtual uint64_t pos() const = 0;

            //! Get file size
            virtual uint64_t size() const = 0;

            //! Set file position
            virtual void seek(uint64_t pos) = 0;

            //---

            //! Get file flags
            INLINE uint32_t flags() const { return m_flags; }

            //! Get archive's version
            INLINE uint32_t version() const { return m_version; }

            //! Are we in error state ?
            INLINE bool isError() const { return 0 != (m_flags & (uint32_t)BinaryStreamFlags::Error); }

            //! Is this a reader ?
            INLINE bool isReader() const { return 0 != (m_flags & (uint32_t)BinaryStreamFlags::Reader); }

            //! Is this a writer ?
            INLINE bool isWriter() const { return 0 != (m_flags & (uint32_t)BinaryStreamFlags::Writer); }

            //! Is this a memory based file ?
            INLINE bool isMemoryBased() const { return 0 != (m_flags & (uint32_t)BinaryStreamFlags::MemoryBased); }

            //! Is this a null stream ?
            INLINE bool isNull() const { return 0 != (m_flags & (uint32_t)BinaryStreamFlags::NullFile); }

            //! Get last error
            INLINE const StringBuf& lastError() const { return m_lastError; }

            //---

            //! Signal file error, all IO errors are reported
            void reportError(const char* txt);

            //! Clear file error, use with care
            void clearErorr();

        private:
            StringBuf m_lastError;
        };

    } // stream

} //  base