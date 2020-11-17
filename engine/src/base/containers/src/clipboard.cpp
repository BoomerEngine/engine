/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "clipboard.h"
#include "stringBuf.h"

namespace base
{
    //---

    ClipboardData::ClipboardData()
    {}

    ClipboardData::ClipboardData(const StringBuf& txt)
            : m_type("Text")
    {
        m_data = txt.uni_str().view().toBufferWithZero();
    }

    ClipboardData::ClipboardData(const UTF16StringVector& txt)
            : m_type("Text")
    {
        m_data = txt.view().toBufferWithZero();
    }

    ClipboardData::ClipboardData(const Buffer& buffer, StringID type)
            : m_type(type)
            , m_data(buffer)
    {}

    StringBuf ClipboardData::stringDataUTF8() const
    {
        return stringDataUTF16().ansi_str();
    }

    UTF16StringVector ClipboardData::stringDataUTF16() const
    {
        if (!m_data || !isText())
            return UTF16StringVector();

        return UTF16StringVector((const wchar_t*)m_data.data(), (uint32_t)m_data.size() / sizeof(wchar_t));
    }

    //---

    IClipboardHandler::~IClipboardHandler()
    {}

    //---

} // base