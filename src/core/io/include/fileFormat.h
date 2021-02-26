/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#pragma once

#include "core/containers/include/stringBuf.h"

BEGIN_BOOMER_NAMESPACE_EX(io)

/// file format information, extension + description
class FileFormat
{
public:
    INLINE FileFormat()
    {}

    INLINE FileFormat(const FileFormat& other)
        : m_ext(other.m_ext)
        , m_desc(other.m_desc)
    {}

    INLINE FileFormat(FileFormat&& other)
        : m_ext(std::move(other.m_ext))
        , m_desc(std::move(other.m_desc))
    {}

    INLINE FileFormat(const StringBuf& ext, const StringBuf& desc)
        : m_ext(ext)
        , m_desc(desc)
    {}

    INLINE FileFormat& operator=(const FileFormat& other)
    {
        m_ext = other.m_ext;
        m_desc = other.m_desc;
        return *this;
    }

    INLINE FileFormat& operator=(FileFormat&& other)
    {
        m_ext = std::move(other.m_ext);
        m_desc = std::move(other.m_desc);;
        return *this;
    }

    //---

    INLINE const StringBuf& extension() const
    {
        return m_ext;
    }

    INLINE const StringBuf& description() const
    {
        return m_desc;
    }

private:
    StringBuf m_ext;
    StringBuf m_desc;
};

END_BOOMER_NAMESPACE_EX(io)
