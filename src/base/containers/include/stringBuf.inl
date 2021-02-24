/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//---

INLINE void StringBuf::clear()
{
    if (m_data)
    {
        m_data->release();
        m_data = nullptr;
    }
}

INLINE bool StringBuf::empty() const
{
    return !m_data;
}

INLINE uint32_t StringBuf::length() const
{
    return m_data ? m_data->length() : 0;
}

INLINE uint32_t StringBuf::CalcHash(const StringBuf& txt)
{
    return StringView::CalcHash(txt.view());
}

INLINE uint32_t StringBuf::CalcHash(StringView txt)
{
    return StringView::CalcHash(txt);
}

INLINE uint32_t StringBuf::CalcHash(const char* txt)
{
    return StringView::CalcHash(txt);
}

INLINE uint32_t StringBuf::cRC32() const
{
    return view().calcCRC32();
}

INLINE uint64_t StringBuf::cRC64() const
{
    return view().calcCRC64();
}

INLINE const char* StringBuf::c_str() const
{
    return m_data ? m_data->c_str() : "";
}

INLINE StringView StringBuf::view() const
{
    return StringView(c_str(), length());
}

INLINE StringBuf::operator StringView() const
{
    return view();
}

INLINE UTF16StringVector StringBuf::uni_str() const
{
    return UTF16StringVector(c_str());
}

INLINE StringBuf::operator bool() const
{
    return !empty();
}

//---

END_BOOMER_NAMESPACE(base)