/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\output #]
***/

#pragma once

#include "format.h"

BEGIN_BOOMER_NAMESPACE()

//----

// stream utility that writes to the buffer of given size, if needed and allowed buffer can be automatically resized
// NOTE: zero is always written so the content of the buffer can be printed directly via system that expect null-terminated strings
struct CORE_SYSTEM_API BufferStream : public IFormatStream
{
public:
    BufferStream(char* buffer, uint64_t size, bool allowResize = false);
    ~BufferStream();

    //--

    // is the stream empty ?
    INLINE bool empty() const { return m_writePtr == m_startPtr; }

    // is the stream full ?
    INLINE bool full() const { return m_writePtr == m_endPtr; }

    // get length of the data in the stream (NOTE: no trailing zero)
    INLINE uint64_t length() const { return m_writePtr - m_startPtr; }

    // get access to the data
    INLINE const char* data() const { return m_startPtr; }

    // get access to the data
    INLINE const char* c_str() const { return m_startPtr; }

    // reset content
    void clear();

    //--

    // append number of chars to the stream
    virtual IFormatStream& append(const char* str, uint32_t len = INDEX_MAX) override final;

private:
    char* m_startPtr;
    char* m_writePtr;
    char* m_endPtr;
    bool m_allowResize;
    bool m_resized;
};

//----

/// stream with some small initial temporary buffer that can be extended (although in a costly way) when needed
/// NOTE: this class is so basic (before we even declare  namespace) that it will use classic malloc/free
template< uint32_t MAX_SIZE=256 >
struct BaseTempString : public BufferStream
{
public:
    INLINE BaseTempString()
        : BufferStream(m_buffer, MAX_SIZE, true)
    {}

    template< typename... Args>
    INLINE BaseTempString(const char* txt, const Args& ... args)
        : BufferStream(m_buffer, MAX_SIZE, true /* allow resize */)
    {
        innerFormatter(txt, std::forward<const Args&>(args)...);
    }

    // allow to use the temp string as a const char* replacement
    INLINE operator const char*() const { return c_str(); }

private:
    char m_buffer[MAX_SIZE];
};

// sample usage:
// TempBufferStream("Vector = {},{},{}", x, y, z);
// TempBufferStream("").append("This ").append("is ").appendf("example {}", N);

//----

/// default sized temp string buffer
using TempString = BaseTempString<256>;

//---

END_BOOMER_NAMESPACE()
