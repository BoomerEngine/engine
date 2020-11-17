/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#include "build.h"
#include "stringBuilder.h"
#include "base/containers/include/utf8StringFunctions.h"

namespace base
{
    //--

    StringBuilder::StringBuilder()
        : m_buf(m_inlineBuf)
        , m_capacity(INLINE_BUFFER_SIZE)
        , m_length(0)
    {
        writeNullTerminator();
    }

    StringBuilder::~StringBuilder()
    {
        clear();
    }

    void StringBuilder::clear()
    {
        if (m_buf != m_inlineBuf)
            MemFree(m_buf);

        m_buf = m_inlineBuf;
        m_inlineBuf[0] = 0;
        m_capacity = INLINE_BUFFER_SIZE;
        m_length = 0;
    }

    IFormatStream& StringBuilder::append(StringView view)
    {
        if (!view.empty())
        {
            auto requiredCapacity = m_length + view.length() + 1;
            if (ensureCapacity(range_cast<uint32_t>(requiredCapacity)))
            {
                memcpy(m_buf + m_length, view.data(), view.length());
                m_length = range_cast<uint32_t>(m_length + view.length());

                writeNullTerminator();
            }
        }

        return *this;
    }

    IFormatStream& StringBuilder::append(const wchar_t* str, uint32_t len /*= INDEX_MAX*/)
    {
        if (str && *str)
        {
            if (len == INDEX_MAX)
                len = wcslen(str);

            uint32_t convertedLength = 0;
            for (uint32_t i = 0; i < len; ++i)
            {
                char data[6];
                convertedLength += utf8::ConvertChar(data, str[i]);
            }

            // allocate
            auto requiredCapacity = m_length + convertedLength + 1;
            if (ensureCapacity(requiredCapacity))
            {
                for (uint32_t i = 0; i < len; ++i)
                    m_length += utf8::ConvertChar(m_buf + m_length, str[i]);
                writeNullTerminator();
            }
        }

        return *this;
    }

    IFormatStream& StringBuilder::append(const char* str, uint32_t len /*= INDEX_MAX*/)
    {
        return append(StringView(str, len));
    }

    IFormatStream& StringBuilder::append(const StringBuf& str)
    {
        return append(str.c_str());
    }

    IFormatStream& StringBuilder::append(StringID str)
    {
        return append(str.c_str());
    }

    bool StringBuilder::doubleCapacity()
    {
        auto requiredCapacity  = static_cast<uint32_t>(m_capacity * 2);
        return ensureCapacity(requiredCapacity);
    }

    bool StringBuilder::ensureCapacity(uint32_t requiredCapacity)
    {
        auto curCapacity  = m_capacity;
        if (requiredCapacity > m_capacity)
        {
            // grow in orderly fashion
            m_capacity = (m_capacity * 3) / 2;
            if (requiredCapacity > m_capacity)
                m_capacity = requiredCapacity; // use exact given size if it's to big

            // resize the buffer
            if (m_buf != m_inlineBuf)
            {
                // buffer was already allocated, use the realloc
                m_buf = (char*) MemRealloc(POOL_STRINGS, m_buf, sizeof(char) * m_capacity, 1);
            }
            else
            {
                // we are moving out of the internal memory, allocate a new buffer on the heap
                m_buf = (char*) MemAlloc(POOL_STRINGS, sizeof(char) * m_capacity, 1);
                memcpy( m_buf, m_inlineBuf, sizeof(char) * (m_length + 1)); // copy null terminator as well
            }
        }

        return true;
    }

    //--

} // base
