/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\output #]
***/

#include "build.h"
#include "format.h"
#include "formatBuffer.h"

namespace base
{
    //---

    BufferStream::BufferStream(char* buffer, uint64_t size, bool allowResize)
        : m_startPtr(buffer)
        , m_writePtr(buffer)
        , m_endPtr(buffer + size - 1) // -1 so we can always write the zero terminator
        , m_allowResize(allowResize)
        , m_resized(false)
    {
        ASSERT(size >= 1);
        *m_writePtr = 0;
    }

    BufferStream::~BufferStream()
    {
        if (m_resized)
        {
            free(m_startPtr);
            m_resized = false;
            m_startPtr = nullptr;
            m_endPtr = nullptr;
            m_writePtr = nullptr;
        }
    }

    void BufferStream::clear()
    {
        m_writePtr = m_startPtr;
        *m_writePtr = 0;
    }

    IFormatStream& BufferStream::append(const char* str, uint32_t len)
    {
        if (nullptr != str)
        {
            auto writeSize = (len == INDEX_MAX) ? strlen(str) : len;

            // resize if needed and allowed
            if (m_allowResize && (m_writePtr + writeSize >= m_endPtr))
            {
                auto curSize = (size_t)(m_endPtr - m_startPtr);
                auto nextSize = std::max<size_t>(4096 - 64, curSize * 2);
                if (auto newPtr = (char*)malloc(nextSize)) // allocated ?
                {
                    // copy data
                    memcpy(newPtr, m_startPtr, curSize);
                    m_writePtr = newPtr + (m_writePtr - m_startPtr);
                    m_endPtr = newPtr + nextSize - 1;
                    m_startPtr = newPtr;

                    // release previous buffer
                    if (m_resized)
                        free(m_startPtr);

                    // make sure we will free the buffer we just allocated
                    m_resized = true;
                }
            }

            // write text
            auto maxWriteSize = std::min<uint64_t>(m_endPtr - m_writePtr, writeSize);
            memcpy(m_writePtr, str, maxWriteSize);
            m_writePtr += maxWriteSize;
            *m_writePtr = 0;
        }

        return *this;
    }

    //---

} // base
