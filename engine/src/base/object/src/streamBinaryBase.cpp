/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: serialization\stream\binary #]
***/

#include "build.h"
#include "streamBinaryBase.h"
#include "streamBinaryVersion.h"
#include "base/containers/include/stringBuilder.h"

#include <stdarg.h>

namespace base
{
    namespace stream
    {

        IStream::IStream(uint32_t flags)
            : m_flags(flags)
            , m_version(VER_CURRENT)
        {
            DEBUG_CHECK_EX(isReader() || isWriter(), "File must be a reader or a writer");
        }

        IStream::~IStream()
        {}

        void IStream::reportError(const char* txt)
        {
            // file is already in error state
            if (isError())
                return;

            m_lastError = StringBuf(txt);
            m_flags |= (uint32_t)BinaryStreamFlags::Error;

            TRACE_ERROR("Encountered IO error: '{}'", txt);
        }

        void IStream::clearErorr()
        {
            m_lastError = StringBuf();
            m_flags &= ~(uint32_t)BinaryStreamFlags::Error;
        }

    } // stream
} // base
