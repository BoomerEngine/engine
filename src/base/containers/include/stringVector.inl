/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

//-----------------------------------------------------------------------------

template< typename T >
INLINE BaseStringVector<T>::BaseStringVector()
{
}

template< typename T >
INLINE BaseStringVector<T>::BaseStringVector(const BaseStringVector<T>& other)
    : m_buf(other.m_buf)
{
    zeroCheck();
}
template< typename T >
INLINE BaseStringVector<T>::BaseStringVector(BaseStringVector<T>&& other)
    : m_buf(std::move(other.m_buf))
{
    zeroCheck();
}

template< typename T >
INLINE BaseStringVector<T>::BaseStringVector(const char* str, uint32_t length)
{
    prv::Helper<T>::Append(m_buf, BaseStringView<char>(str, length));
    zeroCheck();
}

template< typename T >
INLINE BaseStringVector<T>::BaseStringVector(const wchar_t* str, uint32_t length)
{
    prv::Helper<T>::Append(m_buf, BaseStringView<wchar_t>(str, length));
    zeroCheck();
}

template< typename T >
INLINE BaseStringVector<T>::BaseStringVector(BaseStringView<char> view)
{
    prv::Helper<T>::Append(m_buf, view);
    zeroCheck();
}

template< typename T >
INLINE BaseStringVector<T>::BaseStringVector(const BaseStringView<wchar_t>& view)
{
    prv::Helper<T>::Append(m_buf, view);
    zeroCheck();
}

///---

template< typename T >
INLINE bool BaseStringVector<T>::empty() const
{
    return m_buf.size() <= 1;
}

template< typename T >
INLINE uint32_t BaseStringVector<T>::length() const
{
    return m_buf.size() ? m_buf.size() - 1 : 0;
}

template< typename T >
INLINE BaseStringView<T> BaseStringVector<T>::view() const
{
    return empty() ? BaseStringView<T>() : BaseStringView<T>(m_buf.typedData(), m_buf.typedData() + length());
}

template< typename T >
INLINE BaseStringVector<T>::operator BaseStringView<T>() const
{
    return empty() ? BaseStringView<T>() : BaseStringView<T>(m_buf.typedData(), m_buf.typedData() + length());
}

template< typename T >
INLINE const T* BaseStringVector<T>::c_str() const
{
    static T emptyTable[] = {0};
    return empty() ? emptyTable : m_buf.typedData();
}

template< typename T >
INLINE T* BaseStringVector<T>::c_str_writable()
{
    return empty() ? nullptr : m_buf.typedData();
}

template< typename T >
INLINE void BaseStringVector<T>::print(IFormatStream& f) const
{
    f.append(c_str(), length());
}

template< typename T >
INLINE StringBuf BaseStringVector<T>::ansi_str() const
{
    return StringBuf(view());
}

template< typename T >
INLINE BaseStringVector<T>& BaseStringVector<T>::reserve(uint32_t size)
{
    m_buf.reserve(size);
    return *this;
}

///---

template< typename T >
INLINE BaseStringVector<T>& BaseStringVector<T>::clear()
{
    m_buf.clear();
    return *this;
}

template< typename T >
INLINE BaseStringVector<T>& BaseStringVector<T>::append(const BaseStringVector<T>& str)
{
    return append(str.view());
}

template< typename T >
INLINE BaseStringVector<T>& BaseStringVector<T>::append(BaseStringView<char> str)
{
    prv::Helper<T>::Append(m_buf, BaseStringView<char>(str));
    zeroCheck();
    return *this;
}

template< typename T >
INLINE BaseStringVector<T>& BaseStringVector<T>::append(const BaseStringView<wchar_t>& str)
{
    prv::Helper<T>::Append(m_buf, BaseStringView<wchar_t>(str));
    zeroCheck();
    return *this;
}

///---

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator=(const BaseStringVector<T>& other)
{
    if (this != &other)
        m_buf = other.m_buf;

    return *this;
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator=(BaseStringVector<T>&& other)
{
    if (this != &other)
        m_buf = std::move(other.m_buf);

    return *this;
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator=(BaseStringView<char> str)
{
    clear();
    append(str);
    return *this;
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator=(const BaseStringView<wchar_t>& str)
{
    clear();
    append(str);
    return *this;
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator=(const char* str)
{
    clear();
    append(str);
    return *this;
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator=(const wchar_t* str)
{
    clear();
    append(str);
    return *this;
}

///---

template<typename T>
INLINE bool BaseStringVector<T>::operator==(const BaseStringView<T>& other) const
{
    return view() == other;
}

template<typename T>
INLINE bool BaseStringVector<T>::operator!=(const BaseStringView<T>& other) const
{
    return view() != other;
}

template<typename T>
INLINE bool BaseStringVector<T>::operator<(const BaseStringView<T>& other) const
{
    return view() < other;
}

template<typename T>
INLINE bool BaseStringVector<T>::operator<=(const BaseStringView<T>& other) const
{
    return view() <= other;
}

template<typename T>
INLINE bool BaseStringVector<T>::operator>(const BaseStringView<T>& other) const
{
    return view() > other;
}

template<typename T>
INLINE bool BaseStringVector<T>::operator>=(const BaseStringView<T>& other) const
{
    return view() >= other;
}

///---

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator+=(const BaseStringVector<T>& other)
{
    return append(other);
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator+=(const char* other)
{
    return append(other);
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator+=(const wchar_t* other)
{
    return append(other);
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator+=(BaseStringView<char> other)
{
    return append(other);
}

template<typename T>
INLINE BaseStringVector<T>& BaseStringVector<T>::operator+=(const BaseStringView<wchar_t>& other)
{
    return append(other);
}

//-----------------------------------------------------------------------------

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::operator+(const BaseStringVector<T>& other) const
{
    return BaseStringVector<T>(*this) += other;
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::operator+(const char* other) const
{
    return BaseStringVector<T>(*this) += other;
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::operator+(const wchar_t* other) const
{
    return BaseStringVector<T>(*this) += other;
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::operator+(BaseStringView<char> other) const
{
    return BaseStringVector<T>(*this) += other;
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::operator+(const BaseStringView<wchar_t>& other) const
{
    return BaseStringVector<T>(*this) += other;
}

//-----------------------------------------------------------------------------

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::toLower() const
{
    BaseStringVector<T> ret(*this);

    for (auto& ch : ret.m_buf)
        if (ch >= 'A' && ch <= 'Z')
            ch += 'a' - 'A';

    return ret;
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::toUpper() const
{
    BaseStringVector<T> ret(*this);

    for (auto& ch : ret.m_buf)
        if (ch >= 'a' && ch <= 'z')
            ch += 'A' - 'a';

    return ret;
}

//-----------------------------------------------------------------------------

template<typename T>
INLINE int BaseStringVector<T>::compareWith(const BaseStringView<T>& other) const
{
    return view().cmp(other);
}

template<typename T>
INLINE int BaseStringVector<T>::compareWithNoCase(const BaseStringView<T>& other) const
{
    return view().caseCmp(other);
}

//-----------------------------------------------------------------------------

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::leftPart(uint32_t count) const
{
    return BaseStringVector<T>(view().leftPart(count));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::rightPart(uint32_t count) const
{
    return BaseStringVector<T>(view().rightPart(count));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::subString(uint32_t first, uint32_t count) const
{
    return BaseStringVector<T>(view().subString(count));
}

template<typename T>
INLINE void BaseStringVector<T>::split(uint32_t index, BaseStringVector<T>& outLeft, BaseStringVector<T>& outRight) const
{
    BaseStringView<T> leftPart, rightPart;
    view().split(index, leftPart, rightPart);
    outLeft = BaseStringVector<T>(leftPart);
    outRight = BaseStringVector<T>(rightPart);
}

template<typename T>
INLINE bool BaseStringVector<T>::splitAt(const BaseStringView<T>& str, BaseStringVector<T>& outLeft, BaseStringVector<T>& outRight) const
{
    BaseStringView<T> leftPart, rightPart;
    if (!view().splitAt(str, leftPart, rightPart))
        return false;

    outLeft = BaseStringVector<T>(leftPart);
    outRight = BaseStringVector<T>(rightPart);
    return true;
}

//-----------------------------------------------------------------------------

template<typename T>
INLINE int BaseStringVector<T>::findStr(const BaseStringView<T>& pattern, int firstPosition) const
{
    return view().findStr(pattern, firstPosition);
}

template<typename T>
INLINE int BaseStringVector<T>::findStrRev(const BaseStringView<T>& pattern, int firstPosition) const
{
    return view().findRevStr(pattern, firstPosition);
}

template<typename T>
INLINE int BaseStringVector<T>::findStrNoCase(const BaseStringView<T>& pattern, int firstPosition) const
{
    return view().findStrNoCase(pattern, firstPosition);
}

template<typename T>
INLINE int BaseStringVector<T>::findStrRevNoCase(const BaseStringView<T>& pattern, int firstPosition) const
{
    return view().findRevStrNoCase(pattern, firstPosition);
}

template<typename T>
INLINE int BaseStringVector<T>::findFirstChar(T ch) const
{
    return view().findFirstChar(ch);
}

template<typename T>
INLINE int BaseStringVector<T>::findLastChar(T ch) const
{
    return view().findLastChar(ch);
}

template<typename T>
INLINE bool BaseStringVector<T>::beginsWith(const BaseStringView<T>& pattern) const
{
    return view().beginsWith(pattern);
}

template<typename T>
INLINE bool BaseStringVector<T>::beginsWithNoCase(const BaseStringView<T>& pattern) const
{
    return view().beginsWithNoCase(pattern);
}

template<typename T>
INLINE bool BaseStringVector<T>::endsWith(const BaseStringView<T>& pattern) const
{
    return view().endsWith(pattern);
}

template<typename T>
INLINE bool BaseStringVector<T>::endsWithNoCase(const BaseStringView<T>& pattern) const
{
    return view().endsWithNoCase(pattern);
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::stringAfterFirst(const BaseStringView<T>& pattern) const
{
    return BaseStringVector<T>(view().afterFirst(pattern));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::stringBeforeFirst(const BaseStringView<T>& pattern) const
{
    return BaseStringVector<T>(view().beforeFirst(pattern));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::stringAfterLast(const BaseStringView<T>& pattern) const
{
    return BaseStringVector<T>(view().afterLast(pattern));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::stringBeforeLast(const BaseStringView<T>& pattern) const
{
    return BaseStringVector<T>(view().beforeLast(pattern));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::stringAfterFirstNoCase(const BaseStringView<T>& pattern) const
{
    return BaseStringVector<T>(view().afterFirstNoCase(pattern));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::stringBeforeFirstNoCase(const BaseStringView<T>& pattern) const
{
    return BaseStringVector<T>(view().beforeFirstNoCase(pattern));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::stringAfterLastNoCase(const BaseStringView<T>& pattern) const
{
    return BaseStringVector<T>(view().afterLastNoCase(pattern));
}

template<typename T>
INLINE BaseStringVector<T> BaseStringVector<T>::stringBeforeLastNoCase(const BaseStringView<T>& pattern) const
{
    return BaseStringVector<T>(view().beforeLastNoCase(pattern));
}

//-----------------------------------------------------------------------------

template<typename T>
INLINE void BaseStringVector<T>::zeroCheck()
{
    ASSERT(m_buf.empty() || m_buf.back() == 0);
}

//-----------------------------------------------------------------------------

template<typename T>
INLINE ArrayIterator<T> BaseStringVector<T>::begin()
{
    return m_buf.begin();
}

template<typename T>
INLINE ArrayIterator<T> BaseStringVector<T>::end()
{
    return m_buf.end();
}

template<typename T>
INLINE ConstArrayIterator<T> BaseStringVector<T>::begin() const
{
    return m_buf.begin();
}

template<typename T>
INLINE ConstArrayIterator<T> BaseStringVector<T>::end() const
{
    return m_buf.end();
}

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE(base)