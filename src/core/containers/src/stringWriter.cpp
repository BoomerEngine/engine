/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#include "build.h"
#include "stringWriter.h"

#include "core/containers/include/utf8StringFunctions.h"

BEGIN_BOOMER_NAMESPACE()

//--

StringWriter::StringWriter()
{
}

StringWriter::~StringWriter()
{
    ASSERT_EX(m_size == 0, "Not all data was written to file");
}

IFormatStream& StringWriter::append(StringView view)
{
    if (!view.empty())
        append(view.data(), view.length());
    return *this;
}

IFormatStream& StringWriter::append(const StringBuf& str)
{
    return append(str.c_str());
}

IFormatStream& StringWriter::append(StringID str)
{
    return append(str.c_str());
}

void StringWriter::flush()
{
    if (m_size)
    {
        writeData(m_internalBuffer, m_size);
        m_size = 0;
    }
}

IFormatStream& StringWriter::append(const char* str, uint32_t len /*= INDEX_MAX*/)
{
    if (!str || !*str)
        return *this;

    if (len == INDEX_MAX)
        len = strlen(str);

    const auto* endPtr = str + len;
    while (str < endPtr)
    {
        auto remainingSize = INTERNAL_BUFFER_SIZE - m_size;
        auto copySize = std::min<uint64_t>(endPtr - str, remainingSize);

        if (copySize == 0)
        {
            flush();
            continue;
        }

        memcpy(m_internalBuffer + m_size, str, copySize);
        str += copySize;
        m_size += copySize;
    }

    ASSERT(str == endPtr);
    return *this;
}

IFormatStream& StringWriter::append(const wchar_t* str, uint32_t len /*= INDEX_MAX*/)
{
    if (!str || !*str)
        return *this;

    if (len == INDEX_MAX)
        len = wcslen(str);

    const auto* endPtr = str + len;
    while (str < endPtr)
    {
        auto remainingSize = INTERNAL_BUFFER_SIZE - m_size;

        // measure how much can we write
        uint32_t convertedLength = 0;
        const auto* start = str;
        while (str < endPtr)
        {
            char data[6];
            const auto charSize = utf8::ConvertChar(data, *str);

            if (convertedLength + charSize > remainingSize)
                break;

            convertedLength += charSize;
            str += 1;
        }

        // flush data when we can't write any more
        if (convertedLength == 0)
        {
            flush();
            continue;
        }

        // write chars
        for (const auto* read = start; read < str; ++read)
        {
            const auto charSize = utf8::ConvertChar(m_internalBuffer + m_size, *read);
            m_size += charSize;
            ASSERT(m_size <= INTERNAL_BUFFER_SIZE);
        }
    }

    return *this;
}

//--

END_BOOMER_NAMESPACE()

