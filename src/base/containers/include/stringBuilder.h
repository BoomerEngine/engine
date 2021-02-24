/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

#include "stringBuf.h"

BEGIN_BOOMER_NAMESPACE(base)

/// a helper class that can build long strings in a safe way
class BASE_CONTAINERS_API StringBuilder : public IFormatStream
{
public:
    StringBuilder();
    virtual ~StringBuilder();

    //---

    // clear builder
    void clear();

    // is the builder empty ?
    INLINE bool empty() const { return m_length == 0; }

    // get size of the data
    INLINE uint32_t length() const { return m_length; }

    // get as string (makes a copy)
    INLINE StringBuf toString() const { checkNullTerminator(); return StringBuf(m_buf); }

    // make a view
    INLINE StringView view() const { checkNullTerminator(); return StringView(m_buf, m_length); }

    // get data
    INLINE char* c_str() const { checkNullTerminator(); return m_buf; }

    //---

    IFormatStream& append(StringView view);
    IFormatStream& append(const StringBuf& str);
    IFormatStream& append(StringID str);

    //---

    // append number of chars to the stream
    virtual IFormatStream& append(const char* str, uint32_t len = INDEX_MAX) override final;

    // append wide-char stream
    virtual IFormatStream& append(const wchar_t* str, uint32_t len = INDEX_MAX) override final;

private:
    static const uint32_t INLINE_BUFFER_SIZE = 256;

    INLINE void checkNullTerminator() const
    {
        DEBUG_CHECK_EX(m_buf[m_length] == 0, "");
    }

    INLINE void writeNullTerminator()
    {
        DEBUG_CHECK_EX(m_length < m_capacity, "run out of space");
        m_buf[m_length] = 0;
    }

    // make sure we have at least this amount of space
    bool ensureCapacity(uint32_t requiredCapacity);

    // double existing capacity by allocating larger buffer
    bool doubleCapacity();

    char m_inlineBuf[INLINE_BUFFER_SIZE];
    char* m_buf;
    uint32_t m_capacity;
    uint32_t m_length;
};

END_BOOMER_NAMESPACE(base)
