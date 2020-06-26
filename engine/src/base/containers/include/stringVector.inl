/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

namespace base
{

    //-----------------------------------------------------------------------------

    template< typename T >
    INLINE StringVector<T>::StringVector()
    {
    }

    template< typename T >
    INLINE StringVector<T>::StringVector(const StringVector<T>& other)
        : m_buf(other.m_buf)
    {
        zeroCheck();
    }
    template< typename T >
    INLINE StringVector<T>::StringVector(StringVector<T>&& other)
        : m_buf(std::move(other.m_buf))
    {
        zeroCheck();
    }

    template< typename T >
    INLINE StringVector<T>::StringVector(const char* str, uint32_t length)
    {
        prv::Helper<T>::Append(m_buf, StringView<char>(str, length));
        zeroCheck();
    }

    template< typename T >
    INLINE StringVector<T>::StringVector(const wchar_t* str, uint32_t length)
    {
        prv::Helper<T>::Append(m_buf, StringView<wchar_t>(str, length));
        zeroCheck();
    }

    template< typename T >
    INLINE StringVector<T>::StringVector(StringView<char> view)
    {
        prv::Helper<T>::Append(m_buf, view);
        zeroCheck();
    }

    template< typename T >
    INLINE StringVector<T>::StringVector(const StringView<wchar_t>& view)
    {
        prv::Helper<T>::Append(m_buf, view);
        zeroCheck();
    }

    ///---

    template< typename T >
    INLINE bool StringVector<T>::empty() const
    {
        return m_buf.size() <= 1;
    }

    template< typename T >
    INLINE uint32_t StringVector<T>::length() const
    {
        return m_buf.size() ? m_buf.size() - 1 : 0;
    }

    template< typename T >
    INLINE StringView<T> StringVector<T>::view() const
    {
        return empty() ? StringView<T>() : StringView<T>(m_buf.typedData(), m_buf.typedData() + length());
    }

    template< typename T >
    INLINE StringVector<T>::operator StringView<T>() const
    {
        return empty() ? StringView<T>() : StringView<T>(m_buf.typedData(), m_buf.typedData() + length());
    }

    template< typename T >
    INLINE const T* StringVector<T>::c_str() const
    {
        static T emptyTable[] = {0};
        return empty() ? emptyTable : m_buf.typedData();
    }

    template< typename T >
    INLINE T* StringVector<T>::c_str()
    {
        return empty() ? nullptr : m_buf.typedData();
    }

    template< typename T >
    INLINE void StringVector<T>::print(IFormatStream& f) const
    {
        f.append(c_str(), length());
    }

    template< typename T >
    INLINE StringBuf StringVector<T>::ansi_str() const
    {
        return StringBuf(view());
    }

    ///---

    template< typename T >
    INLINE StringVector<T>& StringVector<T>::clear()
    {
        m_buf.clear();
        return *this;
    }

    template< typename T >
    INLINE StringVector<T>& StringVector<T>::append(const StringVector<T>& str)
    {
        return append(str.view());
    }

    template< typename T >
    INLINE StringVector<T>& StringVector<T>::append(StringView<char> str)
    {
        prv::Helper<T>::Append(m_buf, StringView<char>(str));
        zeroCheck();
        return *this;
    }

    template< typename T >
    INLINE StringVector<T>& StringVector<T>::append(const StringView<wchar_t>& str)
    {
        prv::Helper<T>::Append(m_buf, StringView<wchar_t>(str));
        zeroCheck();
        return *this;
    }

    ///---

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator=(const StringVector<T>& other)
    {
        if (this != &other)
            m_buf = other.m_buf;

        return *this;
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator=(StringVector<T>&& other)
    {
        if (this != &other)
            m_buf = std::move(other.m_buf);

        return *this;
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator=(StringView<char> str)
    {
        clear();
        append(str);
        return *this;
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator=(const StringView<wchar_t>& str)
    {
        clear();
        append(str);
        return *this;
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator=(const char* str)
    {
        clear();
        append(str);
        return *this;
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator=(const wchar_t* str)
    {
        clear();
        append(str);
        return *this;
    }

    ///---

    template<typename T>
    INLINE bool StringVector<T>::operator==(const StringView<T>& other) const
    {
        return view() == other;
    }

    template<typename T>
    INLINE bool StringVector<T>::operator!=(const StringView<T>& other) const
    {
        return view() != other;
    }

    template<typename T>
    INLINE bool StringVector<T>::operator<(const StringView<T>& other) const
    {
        return view() < other;
    }

    template<typename T>
    INLINE bool StringVector<T>::operator<=(const StringView<T>& other) const
    {
        return view() <= other;
    }

    template<typename T>
    INLINE bool StringVector<T>::operator>(const StringView<T>& other) const
    {
        return view() > other;
    }

    template<typename T>
    INLINE bool StringVector<T>::operator>=(const StringView<T>& other) const
    {
        return view() >= other;
    }

    ///---

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator+=(const StringVector<T>& other)
    {
        return append(other);
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator+=(const char* other)
    {
        return append(other);
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator+=(const wchar_t* other)
    {
        return append(other);
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator+=(StringView<char> other)
    {
        return append(other);
    }

    template<typename T>
    INLINE StringVector<T>& StringVector<T>::operator+=(const StringView<wchar_t>& other)
    {
        return append(other);
    }

    //-----------------------------------------------------------------------------

    template<typename T>
    INLINE StringVector<T> StringVector<T>::operator+(const StringVector<T>& other) const
    {
        return StringVector<T>(*this) += other;
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::operator+(const char* other) const
    {
        return StringVector<T>(*this) += other;
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::operator+(const wchar_t* other) const
    {
        return StringVector<T>(*this) += other;
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::operator+(StringView<char> other) const
    {
        return StringVector<T>(*this) += other;
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::operator+(const StringView<wchar_t>& other) const
    {
        return StringVector<T>(*this) += other;
    }

    //-----------------------------------------------------------------------------

    template<typename T>
    INLINE StringVector<T> StringVector<T>::toLower() const
    {
        StringVector<T> ret(*this);

        for (auto& ch : ret.m_buf)
            if (ch >= 'A' && ch <= 'Z')
                ch += 'a' - 'A';

        return ret;
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::toUpper() const
    {
        StringVector<T> ret(*this);

        for (auto& ch : ret.m_buf)
            if (ch >= 'a' && ch <= 'z')
                ch += 'A' - 'a';

        return ret;
    }

    //-----------------------------------------------------------------------------

    template<typename T>
    INLINE int StringVector<T>::compareWith(const StringView<T>& other) const
    {
        return view().cmp(other);
    }

    template<typename T>
    INLINE int StringVector<T>::compareWithNoCase(const StringView<T>& other) const
    {
        return view().caseCmp(other);
    }

    //-----------------------------------------------------------------------------

    template<typename T>
    INLINE StringVector<T> StringVector<T>::leftPart(uint32_t count) const
    {
        return StringVector<T>(view().leftPart(count));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::rightPart(uint32_t count) const
    {
        return StringVector<T>(view().rightPart(count));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::subString(uint32_t first, uint32_t count) const
    {
        return StringVector<T>(view().subString(count));
    }

    template<typename T>
    INLINE void StringVector<T>::split(uint32_t index, StringVector<T>& outLeft, StringVector<T>& outRight) const
    {
        StringView<T> leftPart, rightPart;
        view().split(index, leftPart, rightPart);
        outLeft = StringVector<T>(leftPart);
        outRight = StringVector<T>(rightPart);
    }

    template<typename T>
    INLINE bool StringVector<T>::splitAt(const StringView<T>& str, StringVector<T>& outLeft, StringVector<T>& outRight) const
    {
        StringView<T> leftPart, rightPart;
        if (!view().splitAt(str, leftPart, rightPart))
            return false;

        outLeft = StringVector<T>(leftPart);
        outRight = StringVector<T>(rightPart);
        return true;
    }

    //-----------------------------------------------------------------------------

    template<typename T>
    INLINE int StringVector<T>::findStr(const StringView<T>& pattern, int firstPosition) const
    {
        return view().findStr(pattern, firstPosition);
    }

    template<typename T>
    INLINE int StringVector<T>::findStrRev(const StringView<T>& pattern, int firstPosition) const
    {
        return view().findRevStr(pattern, firstPosition);
    }

    template<typename T>
    INLINE int StringVector<T>::findStrNoCase(const StringView<T>& pattern, int firstPosition) const
    {
        return view().findStrNoCase(pattern, firstPosition);
    }

    template<typename T>
    INLINE int StringVector<T>::findStrRevNoCase(const StringView<T>& pattern, int firstPosition) const
    {
        return view().findRevStrNoCase(pattern, firstPosition);
    }

    template<typename T>
    INLINE int StringVector<T>::findFirstChar(T ch) const
    {
        return view().findFirstChar(ch);
    }

    template<typename T>
    INLINE int StringVector<T>::findLastChar(T ch) const
    {
        return view().findLastChar(ch);
    }

    template<typename T>
    INLINE bool StringVector<T>::beginsWith(const StringView<T>& pattern) const
    {
        return view().beginsWith(pattern);
    }

    template<typename T>
    INLINE bool StringVector<T>::beginsWithNoCase(const StringView<T>& pattern) const
    {
        return view().beginsWithNoCase(pattern);
    }

    template<typename T>
    INLINE bool StringVector<T>::endsWith(const StringView<T>& pattern) const
    {
        return view().endsWith(pattern);
    }

    template<typename T>
    INLINE bool StringVector<T>::endsWithNoCase(const StringView<T>& pattern) const
    {
        return view().endsWithNoCase(pattern);
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::stringAfterFirst(const StringView<T>& pattern) const
    {
        return StringVector<T>(view().afterFirst(pattern));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::stringBeforeFirst(const StringView<T>& pattern) const
    {
        return StringVector<T>(view().beforeFirst(pattern));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::stringAfterLast(const StringView<T>& pattern) const
    {
        return StringVector<T>(view().afterLast(pattern));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::stringBeforeLast(const StringView<T>& pattern) const
    {
        return StringVector<T>(view().beforeLast(pattern));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::stringAfterFirstNoCase(const StringView<T>& pattern) const
    {
        return StringVector<T>(view().afterFirstNoCase(pattern));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::stringBeforeFirstNoCase(const StringView<T>& pattern) const
    {
        return StringVector<T>(view().beforeFirstNoCase(pattern));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::stringAfterLastNoCase(const StringView<T>& pattern) const
    {
        return StringVector<T>(view().afterLastNoCase(pattern));
    }

    template<typename T>
    INLINE StringVector<T> StringVector<T>::stringBeforeLastNoCase(const StringView<T>& pattern) const
    {
        return StringVector<T>(view().beforeLastNoCase(pattern));
    }

    //-----------------------------------------------------------------------------

    template<typename T>
    INLINE void StringVector<T>::zeroCheck()
    {
        ASSERT(m_buf.empty() || m_buf.back() == 0);
    }

    //-----------------------------------------------------------------------------

    template<typename T>
    INLINE ArrayIterator<T> StringVector<T>::begin()
    {
        return m_buf.begin();
    }

    template<typename T>
    INLINE ArrayIterator<T> StringVector<T>::end()
    {
        return m_buf.end();
    }

    template<typename T>
    INLINE ConstArrayIterator<T> StringVector<T>::begin() const
    {
        return m_buf.begin();
    }

    template<typename T>
    INLINE ConstArrayIterator<T> StringVector<T>::end() const
    {
        return m_buf.end();
    }

    //-----------------------------------------------------------------------------

} // base