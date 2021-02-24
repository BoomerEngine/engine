/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string\utf16 #]
***/

#pragma once

BEGIN_BOOMER_NAMESPACE(base)

namespace prv
{

    struct BASE_CONTAINERS_API BaseHelper
    {
        static void Slice(const char* data, uint64_t length, const char* splitChars, bool keepEmpty, Array< BaseStringView<char> >& outTokens);
        static void Slice(const wchar_t* data, uint64_t length, const char* splitChars, bool keepEmpty, Array< BaseStringView<wchar_t> >& outTokens);

        static int64_t Find(const char* haystack, uint64_t length, const char* key, uint64_t keyLength);
        static int64_t Find(const wchar_t* haystack, uint64_t length, const wchar_t* key, uint64_t keyLength);

        static int64_t FindNoCase(const char* haystack, uint64_t length, const char* key, uint64_t keyLength);
        static int64_t FindNoCase(const wchar_t* haystack, uint64_t length, const wchar_t* key, uint64_t keyLength);

        static int64_t FindRev(const char* haystack, uint64_t length, const char* key, uint64_t keyLength);
        static int64_t FindRev(const wchar_t* haystack, uint64_t length, const wchar_t* key, uint64_t keyLength);

        static int64_t FindRevNoCase(const char* haystack, uint64_t length, const char* key, uint64_t keyLength);
        static int64_t FindRevNoCase(const wchar_t* haystack, uint64_t length, const wchar_t* key, uint64_t keyLength);

        static uint64_t StringHash(const char* str, const char* end);
        static uint64_t StringHashNoCase(const char* str, const char* end);
        static uint32_t StringCRC32(const char* str, const char* end, uint32_t crc);
        static uint64_t StringCRC64(const char* str, const char* end, uint64_t crc);

        static uint64_t StringHash(const wchar_t* str, const wchar_t* end);
        static uint32_t StringCRC32(const wchar_t* str, const wchar_t* end, uint32_t crc);
        static uint64_t StringCRC64(const wchar_t* str, const wchar_t* end, uint64_t crc);

        static void Append(Array<char>& table, BaseStringView<char> view);
        static void Append(Array<wchar_t>& table, BaseStringView<char> view);
        static void Append(Array<char>& table, const BaseStringView<wchar_t>& view);
        static void Append(Array<wchar_t>& table, const BaseStringView<wchar_t>& view);

        static bool MatchPattern(BaseStringView<char> str, BaseStringView<char> pattern);
        static bool MatchPattern(const BaseStringView<wchar_t>& str, const BaseStringView<wchar_t>& pattern);
        static bool MatchPatternNoCase(BaseStringView<char> str, BaseStringView<char> pattern);
        static bool MatchPatternNoCase(const BaseStringView<wchar_t>& str, const BaseStringView<wchar_t>& pattern);

        static bool MatchString(BaseStringView<char> str, BaseStringView<char> pattern);
        static bool MatchString(const BaseStringView<wchar_t>& str, const BaseStringView<wchar_t>& pattern);
        static bool MatchStringNoCase(BaseStringView<char> str, BaseStringView<char> pattern);
        static bool MatchStringNoCase(const BaseStringView<wchar_t>& str, const BaseStringView<wchar_t>& pattern);

        static MatchResult MatchInteger(const char* str, uint8_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const char* str, uint16_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const char* str, uint32_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const char* str, uint64_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const char* str, char& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const char* str, short& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const char* str, int& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const char* str, int64_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const wchar_t* str, uint8_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const wchar_t* str, uint16_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const wchar_t* str, uint32_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const wchar_t* str, uint64_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const wchar_t* str, char& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const wchar_t* str, short& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const wchar_t* str, int& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);
        static MatchResult MatchInteger(const wchar_t* str, int64_t& outValue, size_t strLength = MAX_SIZE_T, uint32_t base = 10);

        static MatchResult MatchFloat(const char* str, double& outValue, size_t strLength = MAX_SIZE_T);
        static MatchResult MatchFloat(const wchar_t* str, double& outValue, size_t strLength = MAX_SIZE_T);

    };

    template< typename T >
    struct Helper : BaseHelper
    {};

    template<>
    struct Helper<char> : BaseHelper
    {
        static INLINE size_t Length(const char* txt)
        {
            return strlen(txt);
        }

        static INLINE int Compare(const char* a, const char* b, size_t length)
        {
            return strncmp(a, b, length);
        }

        static INLINE int CaseCompare(const char* a, const char* b, size_t length)
        {
            return _strnicmp(a, b, length);
        }
    };

    template<>
    struct Helper<wchar_t> : BaseHelper
    {
        static INLINE size_t Length(const wchar_t* txt)
        {
            return wcslen(txt);
        }

        static INLINE int Compare(const wchar_t* a, const wchar_t* b, size_t length)
        {
            return wcsncmp(a, b, length);
        }

        static INLINE int CaseCompare(const wchar_t* a, const wchar_t* b, size_t length)
        {
#ifdef PLATFORM_MSVC
			return _wcsnicmp(a, b, length);
#else
                return wcsncasecmp(a, b, length);
#endif
        }
    };

} // prv

template< typename T >
INLINE BaseStringView<T>::BaseStringView()
    : m_start(nullptr)
    , m_end(nullptr)
{}

template< typename T >
INLINE BaseStringView<T>::BaseStringView(const T* start, uint32_t length/* = INDEX_MAX*/)
    : m_start(start)
    , m_end(start)
{
    if (start)
    {
        auto len = (length == INDEX_MAX) ? prv::Helper<T>::Length(start) : length;
        m_end = m_start + len;
    }
}

template< typename T >
INLINE BaseStringView<T>::BaseStringView(const ConstArrayIterator<T>& start, const ConstArrayIterator<T>& end)
    : m_start(start.ptr())
    , m_end(end.ptr())
{}

template< typename T >
INLINE BaseStringView<T>::BaseStringView(const T* start, const T* end)
    : m_start(start)
    , m_end(end)
{}

template< typename T >
INLINE BaseStringView<T>::BaseStringView(const BaseStringView<T>& other)
    : m_start(other.m_start)
    , m_end(other.m_end)
{}

template< typename T >
INLINE BaseStringView<T>::BaseStringView(BaseStringView&& other)
{
    m_start = other.m_start;
    m_end = other.m_end;
    other.m_start = nullptr;
    other.m_end = nullptr;
}

template< typename T >
template< uint32_t N >
INLINE BaseStringView<T>::BaseStringView(const BaseTempString<N>& tempString)
{
    m_start = tempString.c_str();
    m_end = m_start + prv::Helper<T>::Length(m_start);
}

template< typename T >
INLINE BaseStringView<T>::BaseStringView(const Buffer& rawData)
    : m_start(nullptr)
    , m_end(nullptr)
{
    if (rawData)
    {
        m_start = (const char*)rawData.data();
        m_end = m_start + rawData.size();
    }
}

template< typename T >
INLINE BaseStringView<T>& BaseStringView<T>::operator=(const BaseStringView& other)
{
    m_start = other.m_start;
    m_end = other.m_end;
    return *this;
}

template< typename T >
INLINE BaseStringView<T>& BaseStringView<T>::operator=(BaseStringView&& other)
{
    m_start = other.m_start;
    m_end = other.m_end;
    other.m_start = nullptr;
    other.m_end = nullptr;
    return *this;
}

template< typename T >
INLINE BaseStringView<T>::~BaseStringView()
{
    m_start = nullptr;
    m_end = nullptr;
}

template< typename T >
INLINE bool BaseStringView<T>::empty() const
{
    return (m_start == m_end);
}

template< typename T >
INLINE BaseStringView<T>::operator bool() const
{
    return m_end > m_start;
}

template< typename T >
INLINE uint32_t BaseStringView<T>::length() const
{
    return range_cast<uint32_t>(m_end - m_start);
}

template< typename T >
INLINE const T* BaseStringView<T>::data() const
{
    return m_start;
}

template< typename T >
INLINE void BaseStringView<T>::clear()
{
    m_start = nullptr;
    m_end = nullptr;
}

template< typename T >
INLINE void BaseStringView<T>::print(IFormatStream& p) const
{
    p.append(m_start, length());
}

template< typename T >
INLINE bool BaseStringView<T>::operator==(const BaseStringView<T>& other) const
{
    return 0 == cmp(other);
}

template< typename T >
INLINE bool BaseStringView<T>::operator!=(const BaseStringView<T>& other) const
{
    return 0 != cmp(other);
}

template< typename T >
INLINE bool BaseStringView<T>::operator<(const BaseStringView<T>& other) const
{
    return cmp(other) < 0;
}

template< typename T >
INLINE bool BaseStringView<T>::operator<=(const BaseStringView<T>& other) const
{
    return cmp(other) <= 0;
}

template< typename T >
INLINE bool BaseStringView<T>::operator>(const BaseStringView<T>& other) const
{
    return cmp(other) > 0;
}

template< typename T >
INLINE bool BaseStringView<T>::operator>=(const BaseStringView<T>& other) const
{
    return cmp(other) >= 0;
}

template< typename T >
INLINE int BaseStringView<T>::cmp(const BaseStringView<T>& other, uint64_t count /*= INDEX_MAX*/) const
{
    if (count == INDEX_MAX)
        count = std::max<uint64_t>(length(), other.length());

    // we have enough data
    if (count <= other.length() && count <= length())
        return prv::Helper<T>::Compare(data(), other.data(), count);

    // compare parts
    auto maxCompare = std::min(length(), other.length());
    if (auto ret = prv::Helper<T>::Compare(data(), other.data(), maxCompare))
        return ret;

    // comparable parts were equal, compare sizes
    return (length() < count) ? -1 : 1;
}

template< typename T >
INLINE int BaseStringView<T>::caseCmp(const BaseStringView<T>& other, uint32_t count /*= INDEX_MAX*/) const
{
    if (count == INDEX_MAX)
        count = std::max<uint32_t>(length(), other.length());

    // we have enough data
    if (count <= other.length() && count <= length())
        return prv::Helper<T>::CaseCompare(data(), other.data(), count);

    // compare parts
    auto maxCompare = std::min(length(), other.length());
    if (auto ret = prv::Helper<T>::CaseCompare(data(), other.data(), maxCompare))
        return ret;

    // comparable parts were equal, compare sizes
    return (length() < count) ? -1 : 1;
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::leftPart(uint32_t count /*= INDEX_MAX*/) const
{
    auto max = std::min<uint32_t>(count, length());
	return BaseStringView<T>(m_start, m_start + max);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::rightPart(uint32_t count /*= INDEX_MAX*/) const
{
    auto max = std::min<uint32_t>(count, length());
    return BaseStringView<T>(m_end - max, m_end);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::subString(uint32_t first, uint32_t count /*= INDEX_MAX*/) const
{
    if (first > length()) first = length();
    auto max = std::min(count, length() - first);
    return BaseStringView<T>(m_start + first, m_start + first + max);
}

template< typename T >
INLINE void BaseStringView<T>::split(uint32_t index, BaseStringView<T>& outLeft, BaseStringView<T>& outRight) const
{
    outLeft = leftPart(index);
    outRight = subString(index);
}

template< typename T >
INLINE bool BaseStringView<T>::splitAt(const BaseStringView<T>& str, BaseStringView<T>& outLeft, BaseStringView<T>& outRight) const
{
    auto pos = findStr(str);
    if (pos == INDEX_NONE)
        return false;

    outLeft = leftPart(pos);
    outRight = subString(pos + str.length());
    return true;
}

template< typename T >
INLINE void BaseStringView<T>::slice(const char* splitChars, bool keepEmpty, Array< BaseStringView<T> >& outTokens) const
{
    prv::Helper<T>::Slice(data(), length(), splitChars, keepEmpty, outTokens);
}

template< typename T >
INLINE int BaseStringView<T>::findStr(const BaseStringView& pattern, int firstPosition /*= 0*/) const
{
    if (firstPosition + (int)pattern.length() >= (int)length())
        return -1;

    auto ret = prv::Helper<T>::Find(data() + firstPosition, length() - firstPosition, pattern.data(), pattern.length());
    if (ret != -1)
        ret += firstPosition;

    return range_cast<int>(ret);
}

template< typename T >
INLINE int BaseStringView<T>::findRevStr(const BaseStringView& pattern, int firstPosition /*= 0*/) const
{
    auto searchLimit = std::min<int64_t>(firstPosition, length());
	return range_cast<int>(prv::Helper<T>::FindRev(data(), searchLimit, pattern.data(), pattern.length()));
}

template< typename T >
INLINE int BaseStringView<T>::findStrNoCase(const BaseStringView& pattern, int firstPosition /*= 0*/) const
{
    if (firstPosition + pattern.length() >= (int)length())
    return -1;

    auto ret = prv::Helper<T>::FindNoCase(data() + firstPosition, length() - firstPosition, pattern.data(), pattern.length());
    if (ret != -1)
        ret += firstPosition;

	return range_cast<int>(ret);
}

template< typename T >
INLINE int BaseStringView<T>::findRevStrNoCase(const BaseStringView& pattern, int firstPosition /*= 0*/) const
{
    auto searchLimit = std::min<int64_t>(firstPosition, length());
	return range_cast<int>(prv::Helper<T>::FindRevNoCase(data(), searchLimit, pattern.data(), pattern.length()));
}

template< typename T >
INLINE int BaseStringView<T>::findFirstChar(T ch) const
{
    auto pos  = m_start;
    while (pos < m_end)
    {
        if (*pos == ch)
            return range_cast<int>(pos - m_start);
        pos += 1;
    }

    return -1;
}

template< typename T >
INLINE int BaseStringView<T>::findLastChar(T ch) const
{
    auto pos  = m_end - 1;
    while (pos >= m_start)
    {
        if (*pos == ch)
            return range_cast<int>(pos - m_start);
        pos -= 1;
    }

    return -1;
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::trimLeft() const
{
    auto pos  = m_start;
    while (pos < m_end)
    {
        if (*pos > ' ')
            break;
        pos += 1;
    }

    return BaseStringView<T>(pos, m_end);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::trimRight() const
{
    auto pos  = m_end;
    while (pos > m_start)
    {
        if (pos[-1] > ' ')
            break;
        pos -= 1;
    }

    return BaseStringView<T>(m_start, pos);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::trim() const
{
    return trimLeft().trimRight();
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::trimTailNumbers(uint32_t* outNumber) const
{
    auto pos  = m_end;
    while (pos > m_start)
    {
        if (!(pos[-1] >= '0' && pos[-1] <= '9'))
            break;
        pos -= 1;
    }

    if (pos < m_end && outNumber)
        BaseStringView<T>(pos, m_end).match(*outNumber);

    return BaseStringView<T>(m_start, pos);
}

template< typename T >
INLINE bool BaseStringView<T>::beginsWith(const BaseStringView<T>& pattern) const
{
    return 0 == leftPart(pattern.length()).cmp(pattern);
}

template< typename T >
INLINE bool BaseStringView<T>::beginsWithNoCase(const BaseStringView<T>& pattern) const
{
    return 0 == leftPart(pattern.length()).caseCmp(pattern);
}

template< typename T >
INLINE bool BaseStringView<T>::endsWith(const BaseStringView<T>& pattern) const
{
    return 0 == rightPart(pattern.length()).cmp(pattern);
}

template< typename T >
INLINE bool BaseStringView<T>::endsWithNoCase(const BaseStringView<T>& pattern) const
{
    return 0 == rightPart(pattern.length()).caseCmp(pattern);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::afterFirst(const BaseStringView<T>& pattern) const
{
    auto index = findStr(pattern);
    return (index == -1) ? BaseStringView<T>() : subString(index + pattern.length());
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::afterFirstNoCase(const BaseStringView<T>& pattern) const
{
    auto index = findStrNoCase(pattern);
    return (index == -1) ? BaseStringView<T>() : subString(index + pattern.length());
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::beforeFirst(const BaseStringView<T>& pattern) const
{
    auto index = findStr(pattern);
    return (index == -1) ? BaseStringView<T>() : leftPart(index);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::beforeFirstNoCase(const BaseStringView<T>& pattern) const
{
    auto index = findStrNoCase(pattern);
    return (index == -1) ? BaseStringView<T>() : leftPart(index);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::afterLast(const BaseStringView<T>& pattern) const
{
    auto index = findRevStr(pattern);
    return (index == -1) ? BaseStringView<T>() : subString(index + pattern.length());

}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::afterLastNoCase(const BaseStringView<T>& pattern) const
{
    auto index = findRevStrNoCase(pattern);
    return (index == -1) ? BaseStringView<T>() : subString(index + pattern.length());
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::beforeLast(const BaseStringView<T>& pattern) const
{
    auto index = findRevStr(pattern);
    return (index == -1) ? BaseStringView<T>() : leftPart(index);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::beforeLastNoCase(const BaseStringView<T>& pattern) const
{
    auto index = findRevStrNoCase(pattern);
    return (index == -1) ? BaseStringView<T>() : leftPart(index);
}
    
//--

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::afterFirstOrFull(const BaseStringView<T>& pattern) const
{
    auto index = findStr(pattern);
    return (index == -1) ? *this : subString(index + pattern.length());
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::afterFirstNoCaseOrFull(const BaseStringView<T>& pattern) const
{
    auto index = findStrNoCase(pattern);
    return (index == -1) ? *this : subString(index + pattern.length());
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::beforeFirstOrFull(const BaseStringView<T>& pattern) const
{
    auto index = findStr(pattern);
    return (index == -1) ? *this : leftPart(index);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::beforeFirstNoCaseOrFull(const BaseStringView<T>& pattern) const
{
    auto index = findStrNoCase(pattern);
    return (index == -1) ? *this : leftPart(index);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::afterLastOrFull(const BaseStringView<T>& pattern) const
{
    auto index = findRevStr(pattern);
    return (index == -1) ? *this : subString(index + pattern.length());
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::afterLastNoCaseOrFull(const BaseStringView<T>& pattern) const
{
    auto index = findRevStrNoCase(pattern);
    return (index == -1) ? *this : subString(index + pattern.length());
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::beforeLastOrFull(const BaseStringView<T>& pattern) const
{
    auto index = findRevStr(pattern);
    return (index == -1) ? *this : leftPart(index);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::beforeLastNoCaseOrFull(const BaseStringView<T>& pattern) const
{
    auto index = findRevStrNoCase(pattern);
    return (index == -1) ? *this : leftPart(index);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::fileName() const
{
    const T* fileNameStart = m_start;

    const T* ptr = m_start;
    while (ptr < m_end)
    {
        if (*ptr == '\\' || *ptr == '/')
            fileNameStart = ptr + 1;
        ++ptr;
    }

    return BaseStringView<T>(fileNameStart, m_end);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::fileStem() const
{
    const T dot[2] = { '.', 0 };
    return fileName().beforeFirst(dot);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::extensions() const
{
    const T dot[2] = { '.', 0 };
    return fileName().afterFirst(dot);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::lastExtension() const
{
    const T dot[2] = { '.', 0 };
    return fileName().afterLast(dot);
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::baseDirectory() const
{
    const T* pathEnd = nullptr;

    const T* ptr = m_start;
    while (ptr < m_end)
    {
        if (*ptr == '\\' || *ptr == '/')
            pathEnd = ptr + 1;
        ++ptr;
    }

    if (pathEnd)
        return BaseStringView<T>(m_start, pathEnd);
    else
        return BaseStringView<T>();
}

template< typename T >
INLINE BaseStringView<T> BaseStringView<T>::parentDirectory() const
{
    const T* pathEnd = nullptr;
    const T* prevPathEnd = nullptr;

    const T* ptr = m_start;
    while (ptr < m_end)
    {
        if (*ptr == '\\' || *ptr == '/')
        {
            prevPathEnd = pathEnd;
            pathEnd = ptr + 1;
        }
        ++ptr;
    }

    if (prevPathEnd)
        return BaseStringView<T>(m_start, prevPathEnd);
    else
        return BaseStringView<T>();
}

template< typename T >
INLINE bool BaseStringView<T>::matchString(const BaseStringView<T>& pattern) const
{
    return prv::Helper<T>::MatchString(*this, pattern);
}

template< typename T >
INLINE bool BaseStringView<T>::matchStringNoCase(const BaseStringView<T>& pattern) const
{
    return prv::Helper<T>::MatchStringNoCase(*this, pattern);
}

template< typename T >
INLINE bool BaseStringView<T>::matchPattern(const BaseStringView<T>& pattern) const
{
    return prv::Helper<T>::MatchPattern(*this, pattern);
}

template< typename T >
INLINE bool BaseStringView<T>::matchPatternNoCase(const BaseStringView<T>& pattern) const
{
    return prv::Helper<T>::MatchPatternNoCase(*this, pattern);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(bool& outValue) const
{
    if (*this == "true")
    {
        outValue = true;
        return MatchResult::OK;
    }
    else if (*this == "false")
    {
        outValue = false;
        return MatchResult::OK;
    }

    return MatchResult::InvalidCharacter;
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(uint8_t& outValue, uint32_t base) const
{
    return prv::Helper<T>::MatchInteger(data(), outValue, length(), base);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(uint16_t& outValue, uint32_t base) const
{
    return prv::Helper<T>::MatchInteger(data(), outValue, length(), base);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(uint32_t& outValue, uint32_t base) const
{
    return prv::Helper<T>::MatchInteger(data(), outValue, length(), base);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(uint64_t& outValue, uint32_t base) const
{
    return prv::Helper<T>::MatchInteger(data(), outValue, length(), base);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(char& outValue, uint32_t base) const
{
    return prv::Helper<T>::MatchInteger(data(), outValue, length(), base);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(short& outValue, uint32_t base) const
{
    return prv::Helper<T>::MatchInteger(data(), outValue, length(), base);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(int& outValue, uint32_t base) const
{
    return prv::Helper<T>::MatchInteger(data(), outValue, length(), base);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(int64_t& outValue, uint32_t base) const
{
    return prv::Helper<T>::MatchInteger(data(), outValue, length(), base);
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(float& outValue) const
{
    double result = 0.0f;
    auto ret = prv::Helper<T>::MatchFloat(data(), result, length());
    if (ret == MatchResult::OK)
        outValue = (float)result;
    return ret;
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(double& outValue) const
{
    return prv::Helper<T>::MatchFloat(data(), outValue, length());
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(StringID& outValue) const
{
    outValue = StringID(*this);
    return MatchResult::OK;
}

template< typename T >
INLINE MatchResult BaseStringView<T>::match(StringBuf& outValue) const
{
    outValue = StringBuf(*this);
    return MatchResult::OK;
}

template< typename T >
INLINE uint32_t BaseStringView<T>::CalcHash(BaseStringView<T> txt)
{
    return prv::Helper<T>::StringHash(txt.m_start, txt.m_end);
}

template< typename T >
INLINE uint32_t BaseStringView<T>::calcCRC32(const uint32_t crc/*= CRC32Init*/) const
{
    return prv::Helper<T>::StringCRC32(m_start, m_end, crc);
}

template< typename T >
INLINE uint64_t BaseStringView<T>::calcCRC64(const uint64_t crc/*= CRC64Init*/) const
{
    return prv::Helper<T>::StringCRC64(m_start, m_end, crc);
}

template< typename T >
INLINE Buffer BaseStringView<T>::toBuffer(PoolTag tag) const
{
    if (empty())
        return nullptr;

    return Buffer::Create(tag, length() * sizeof(T), 1, m_start);
}

template< typename T >
INLINE Buffer BaseStringView<T>::toBufferWithZero(PoolTag tag) const
{
    auto size = sizeof(T) * (length() + 1);
	if (size >= INDEX_MAX)
		return nullptr;

    if (!empty() && m_end[-1] == 0)
        return toBuffer(tag);

    if (auto ret = Buffer::Create(tag, length() * sizeof(T) + 1, 1, m_start))
    {
        ((T *) ret.data())[length()] = 0;
        return ret;
    }

    return nullptr;
}

//--

template< typename T >
INLINE ConstArrayIterator<T> BaseStringView<T>::begin() const
{
    return ConstArrayIterator<T>(m_start);
}

template< typename T >
INLINE ConstArrayIterator<T> BaseStringView<T>::end() const
{
    return ConstArrayIterator<T>(m_end);
}

//--

/// a special lower-case only key for hash maps
struct CaseInsensitiveKey
{
    CaseInsensitiveKey(base::BaseStringView<char> view)
    {
        m_data = base::StringBuf(view).toLower();
    }

    CaseInsensitiveKey(const base::StringBuf& txt)
    {
        m_data = txt.toLower();
    }

    static uint32_t CalcHash(const CaseInsensitiveKey& key)
    {
        const auto& txt = key.m_data.view();
        return base::prv::BaseHelper::StringHashNoCase(txt.data(), txt.data() + txt.length());
    }

    static uint32_t CalcHash(base::BaseStringView<char> txt)
    {
        return base::prv::BaseHelper::StringHashNoCase(txt.data(), txt.data() + txt.length());
    }

    INLINE bool operator==(const CaseInsensitiveKey& key) const
    {
        return m_data == key.m_data;
    }

    INLINE bool operator==(base::BaseStringView<char> txt) const
    {
        return 0 == m_data.view().caseCmp(txt);
    }

    base::StringBuf m_data;
};

//--

END_BOOMER_NAMESPACE(base)
