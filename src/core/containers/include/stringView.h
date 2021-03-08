/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string\utf16 #]
***/

#pragma once

#include "core/containers/include/array.h"

BEGIN_BOOMER_NAMESPACE()

//-----------------------------------------------------------------------------

static const uint32_t CRC32Init = 0x0;
static const uint64_t CRC64Init = 0xCBF29CE484222325;

//-----------------------------------------------------------------------------

class StringID;
class StringBuf;

//-----------------------------------------------------------------------------

enum class MatchResult : uint8_t
{
    OK = 0,
    EmptyString = 1,
    InvalidCharacter = 2,
    Overflow = 3,
};

//-----------------------------------------------------------------------------

/// View of a string buffer
template< typename T = char >
class BaseStringView
{
public:
    INLINE BaseStringView();
    INLINE BaseStringView(const T* start, uint32_t length = INDEX_MAX); // computes the length automatically if not provided
    INLINE BaseStringView(const T* start, const T* end);
    INLINE BaseStringView(const ConstArrayIterator<T>& start, const ConstArrayIterator<T>& end);
    INLINE BaseStringView(const BaseStringView<T>& other);
    INLINE BaseStringView(BaseStringView<T>&& other);
    INLINE BaseStringView(const Buffer& rawData);

    template< uint32_t N >
    INLINE BaseStringView(const BaseTempString<N>& tempString);

    INLINE BaseStringView& operator=(const BaseStringView<T>& other);
    INLINE BaseStringView& operator=(BaseStringView<T>&& other);

    INLINE ~BaseStringView();

    //--------------------------

    INLINE const T* data() const;

    INLINE bool empty() const;

    INLINE uint32_t length() const;

    INLINE void clear();

    INLINE void print(IFormatStream& p) const;

    //--------------------------

    INLINE bool operator==(const BaseStringView<T>& other) const;
    INLINE bool operator!=(const BaseStringView<T>& other) const;
    INLINE bool operator<(const BaseStringView<T>& other) const;
    INLINE bool operator<=(const BaseStringView<T>& other) const;
    INLINE bool operator>(const BaseStringView<T>& other) const;
    INLINE bool operator>=(const BaseStringView<T>& other) const;

    //-----------------------------------------------------------------------------

    INLINE operator bool() const;

    //-----------------------------------------------------------------------------

    INLINE int cmp(const BaseStringView<T>& other, uint64_t count = INDEX_MAX) const;

    INLINE int caseCmp(const BaseStringView<T>& other, uint32_t count = INDEX_MAX) const;

    //-----------------------------------------------------------------------------

    // get N left characters, at most to the length of the string
    INLINE BaseStringView<T> leftPart(uint32_t count) const;

    // get N right characters, at most to the length of the string
    INLINE BaseStringView<T> rightPart(uint32_t count) const;

    // get N characters starting at given position, at most till the end of the string
    INLINE BaseStringView<T> subString(uint32_t first, uint32_t count = INDEX_MAX) const;

    // split string into left and right part
    INLINE void split(uint32_t index, BaseStringView<T>& outLeft, BaseStringView<T>& outRight) const;

    // split string at the occurrence of the pattern into left and right part
    INLINE bool splitAt(const BaseStringView<T>& str, BaseStringView<T>& outLeft, BaseStringView<T>& outRight) const;

    /// slice string into parts using the chars from the set
    INLINE void slice(const char* splitChars, bool keepEmpty, Array< BaseStringView >& outTokens) const;

    //-----------------------------------------------------------------------------

    /// trim whitespaces at the start of the string, may return empty string
    INLINE BaseStringView<T> trimLeft() const;

    /// trim whitespaces at the end of the string, may return empty string
    INLINE BaseStringView<T> trimRight() const;

    /// trim whitespaces at both ends
    INLINE BaseStringView<T> trim() const;

    /// trim tailing numbers (Mesh01 -> Mesh), may return an empty string
    INLINE BaseStringView<T> trimTailNumbers(uint32_t* outNumber=nullptr) const;

    //-----------------------------------------------------------------------------

    INLINE int findStr(const BaseStringView& pattern, int firstPosition = 0) const;
    INLINE int findRevStr(const BaseStringView& pattern, int firstPosition = std::numeric_limits<int>::max()) const;
    INLINE int findStrNoCase(const BaseStringView& pattern, int firstPosition = 0) const;
    INLINE int findRevStrNoCase(const BaseStringView& pattern, int firstPosition = std::numeric_limits<int>::max()) const;

    INLINE int findFirstChar(T ch) const;
    INLINE int findLastChar(T ch) const;

    //-----------------------------------------------------------------------------

    INLINE bool beginsWith(const BaseStringView& pattern) const;
    INLINE bool beginsWithNoCase(const BaseStringView& pattern) const;

    INLINE bool endsWith(const BaseStringView& pattern) const;
    INLINE bool endsWithNoCase(const BaseStringView& pattern) const;

    //-----------------------------------------------------------------------------

    INLINE BaseStringView<T> afterFirst(const BaseStringView<T>& pattern) const;
    INLINE BaseStringView<T> afterFirstNoCase(const BaseStringView<T>& pattern) const;

    INLINE BaseStringView<T> beforeFirst(const BaseStringView<T>& pattern) const;
    INLINE BaseStringView<T> beforeFirstNoCase(const BaseStringView<T>& pattern) const;

    INLINE BaseStringView<T> afterLast(const BaseStringView<T>& pattern) const;
    INLINE BaseStringView<T> afterLastNoCase(const BaseStringView<T>& pattern) const;

    INLINE BaseStringView<T> beforeLast(const BaseStringView<T>& pattern) const;
    INLINE BaseStringView<T> beforeLastNoCase(const BaseStringView<T>& pattern) const;

    INLINE BaseStringView<T> afterFirstOrFull(const BaseStringView<T>& pattern) const;
    INLINE BaseStringView<T> afterFirstNoCaseOrFull(const BaseStringView<T>& pattern) const;

    INLINE BaseStringView<T> beforeFirstOrFull(const BaseStringView<T>& pattern) const;
    INLINE BaseStringView<T> beforeFirstNoCaseOrFull(const BaseStringView<T>& pattern) const;

    INLINE BaseStringView<T> afterLastOrFull(const BaseStringView<T>& pattern) const;
    INLINE BaseStringView<T> afterLastNoCaseOrFull(const BaseStringView<T>& pattern) const;

    INLINE BaseStringView<T> beforeLastOrFull(const BaseStringView<T>& pattern) const;
    INLINE BaseStringView<T> beforeLastNoCaseOrFull(const BaseStringView<T>& pattern) const;

    //-----------------------------------------------------------------------------

    INLINE BaseStringView<T> fileName() const; // lena.png.bak (empty for directories)
    INLINE BaseStringView<T> fileStem() const; // lena (not empty for directories)
    INLINE BaseStringView<T> extensions() const; // .png.bak
    INLINE BaseStringView<T> lastExtension() const; // .bak
    INLINE BaseStringView<T> baseDirectory() const; // Z:\test\files\lena.png -> "Z:\test\files\"
    INLINE BaseStringView<T> parentDirectory() const; // Z:\test\files\ -> "Z:\test\"
    INLINE BaseStringView<T> directoryName() const; // Z:\test\files\ -> "files"

    //-----------------------------------------------------------------------------

    INLINE MatchResult match(bool& outValue) const;
    INLINE MatchResult match(uint8_t& outValue, uint32_t base = 10) const;
    INLINE MatchResult match(uint16_t& outValue, uint32_t base = 10) const;
    INLINE MatchResult match(uint32_t& outValue, uint32_t base = 10) const;
    INLINE MatchResult match(uint64_t& outValue, uint32_t base = 10) const;
    INLINE MatchResult match(char& outValue, uint32_t base = 10) const;
    INLINE MatchResult match(short& outValue, uint32_t base = 10) const;
    INLINE MatchResult match(int& outValue, uint32_t base = 10) const;
    INLINE MatchResult match(int64_t& outValue, uint32_t base = 10) const;
    INLINE MatchResult match(float& outValue) const;
    INLINE MatchResult match(double& outValue) const;
    INLINE MatchResult match(StringID& outValue) const;
    INLINE MatchResult match(StringBuf& outValue) const;

    //-----------------------------------------------------------------------------

    // match string to pattern
    INLINE bool matchString(const BaseStringView<T>& pattern) const;
    INLINE bool matchStringNoCase(const BaseStringView<T>& pattern) const;

    // match a pattern or sub string
    INLINE bool matchPattern(const BaseStringView<T>& pattern) const;
    INLINE bool matchPatternNoCase(const BaseStringView<T>& pattern) const;

    //-----------------------------------------------------------------------------

    // compute hash of the string
    INLINE static uint32_t CalcHash(BaseStringView<T> txt);

    // compute 32-bit crc
    INLINE uint32_t calcCRC32(uint32_t crc = CRC32Init) const;

    // compute 64-bit crr
    INLINE uint64_t calcCRC64(uint64_t crc = CRC64Init) const;

    //-----------------------------------------------------------------------------

    // create a memory buffer, NOTE: no null termination
    INLINE Buffer toBuffer(PoolTag tag = POOL_STRINGS) const;

    // create a memory buffer, NOTE: no null termination
    INLINE Buffer toBufferWithZero(PoolTag tag = POOL_STRINGS) const;

    //-----------------------------------------------------------------------------

    //! Get read only iterator to start of the array
    INLINE ConstArrayIterator<T> begin() const;

    //! Get read only iterator to end of the array
    INLINE ConstArrayIterator<T> end() const;

    //-----------------------------------------------------------------------------

private:
    const T* m_start;
    const T* m_end;
};

//-----------------------------------------------------------------------------

// helper class to eat parts of the path from a string
template< typename T = char >
class PathEater
{
public:
    INLINE PathEater(BaseStringView<T> str, bool pureDirPath = false)
    {
        m_pureDirPath = pureDirPath;
        m_begin = str.data();
        m_cur = str.data();
        m_end = str.data() + str.length();
    }

    /// are we at the end ?
    INLINE bool endOfPath() const
    {
        return (m_cur >= m_end);
    }

    /// get rest of the path
    INLINE BaseStringView<char> restOfThePath() const
    {
        return BaseStringView<char>(m_cur, m_end);
    }

    /// reset parsing
    INLINE void reset()
    {
        m_cur = m_begin;
    }

    /// eat next part
    INLINE BaseStringView<char> eatDirectoryName()
    {
        // try to extract stuff
        auto start  = m_cur;
        while (m_cur < m_end)
        {
            // did we get to the part separator ?
            if (*m_cur == '\\' || *m_cur == '/')
            {
                auto length = m_cur - start;
                m_cur += 1;
                return BaseStringView<char>(start, length);
            }

            // go to next char
            m_cur += 1;
        }

        // in the pure path the last entry is a dir as well even without the "/"
        if (m_pureDirPath && m_cur > start && !*m_cur)
            return BaseStringView<char>(start, m_end);

        // not a directory
        m_cur = start;
        return BaseStringView<char>();
    }

private:
    static const uint32_t MAX_PART_SIZE = 512;

    const T* m_cur;
    const T* m_end;
    const T* m_begin;
    bool m_pureDirPath;

    T m_temp[MAX_PART_SIZE];
};

//-----------------------------------------------------------------------------

using StringView = BaseStringView<char>;
using UTF16StringView = BaseStringView<wchar_t>;

//-----------------------------------------------------------------------------

END_BOOMER_NAMESPACE()
