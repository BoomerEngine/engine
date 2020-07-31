/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string\utf16 #]
***/

#pragma once

#include "base/containers/include/array.h"

namespace base
{

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
    class StringView
    {
    public:
        INLINE StringView();
        INLINE StringView(const T* start, uint32_t length = INDEX_MAX); // computes the length automatically if not provided
        INLINE StringView(const T* start, const T* end);
        INLINE StringView(const ConstArrayIterator<T>& start, const ConstArrayIterator<T>& end);
        INLINE StringView(const StringView<T>& other);
        INLINE StringView(StringView<T>&& other);
        INLINE StringView(const Buffer& rawData);

        template< uint32_t N >
        INLINE StringView(const BaseTempString<N>& tempString);

        INLINE StringView& operator=(const StringView<T>& other);
        INLINE StringView& operator=(StringView<T>&& other);

        INLINE ~StringView();

        //--------------------------

        INLINE const T* data() const;

        INLINE bool empty() const;

        INLINE uint32_t length() const;

        INLINE void clear();

        INLINE void print(IFormatStream& p) const;

        //--------------------------

        INLINE bool operator==(const StringView<T>& other) const;
        INLINE bool operator!=(const StringView<T>& other) const;
        INLINE bool operator<(const StringView<T>& other) const;
        INLINE bool operator<=(const StringView<T>& other) const;
        INLINE bool operator>(const StringView<T>& other) const;
        INLINE bool operator>=(const StringView<T>& other) const;

        //-----------------------------------------------------------------------------

        INLINE operator bool() const;

        //-----------------------------------------------------------------------------

        INLINE int cmp(const StringView<T>& other, uint64_t count = INDEX_MAX) const;

        INLINE int caseCmp(const StringView<T>& other, uint32_t count = INDEX_MAX) const;

        //-----------------------------------------------------------------------------

        // get N left characters, at most to the length of the string
        INLINE StringView<T> leftPart(uint32_t count) const;

        // get N right characters, at most to the length of the string
        INLINE StringView<T> rightPart(uint32_t count) const;

        // get N characters starting at given position, at most till the end of the string
        INLINE StringView<T> subString(uint32_t first, uint32_t count = INDEX_MAX) const;

        // split string into left and right part
        INLINE void split(uint32_t index, StringView<T>& outLeft, StringView<T>& outRight) const;

        // split string at the occurrence of the pattern into left and right part
        INLINE bool splitAt(const StringView<T>& str, StringView<T>& outLeft, StringView<T>& outRight) const;

        /// slice string into parts using the chars from the set
        INLINE void slice(const char* splitChars, bool keepEmpty, Array< StringView >& outTokens) const;

        //-----------------------------------------------------------------------------

        /// trim whitespaces at the start of the string, may return empty string
        INLINE StringView<T> trimLeft() const;

        /// trim whitespaces at the end of the string, may return empty string
        INLINE StringView<T> trimRight() const;

        /// trim whitespaces at both ends
        INLINE StringView<T> trim() const;

        /// trim tailing numbers (Mesh01 -> Mesh), may return an empty string
        INLINE StringView<T> trimTailNumbers(uint32_t* outNumber=nullptr) const;

        //-----------------------------------------------------------------------------

        INLINE int findStr(const StringView& pattern, int firstPosition = 0) const;
        INLINE int findRevStr(const StringView& pattern, int firstPosition = std::numeric_limits<int>::max()) const;
        INLINE int findStrNoCase(const StringView& pattern, int firstPosition = 0) const;
        INLINE int findRevStrNoCase(const StringView& pattern, int firstPosition = std::numeric_limits<int>::max()) const;

        INLINE int findFirstChar(T ch) const;
        INLINE int findLastChar(T ch) const;

        //-----------------------------------------------------------------------------

        INLINE bool beginsWith(const StringView& pattern) const;
        INLINE bool beginsWithNoCase(const StringView& pattern) const;

        INLINE bool endsWith(const StringView& pattern) const;
        INLINE bool endsWithNoCase(const StringView& pattern) const;

        //-----------------------------------------------------------------------------

        INLINE StringView<T> afterFirst(const StringView<T>& pattern) const;
        INLINE StringView<T> afterFirstNoCase(const StringView<T>& pattern) const;

        INLINE StringView<T> beforeFirst(const StringView<T>& pattern) const;
        INLINE StringView<T> beforeFirstNoCase(const StringView<T>& pattern) const;

        INLINE StringView<T> afterLast(const StringView<T>& pattern) const;
        INLINE StringView<T> afterLastNoCase(const StringView<T>& pattern) const;

        INLINE StringView<T> beforeLast(const StringView<T>& pattern) const;
        INLINE StringView<T> beforeLastNoCase(const StringView<T>& pattern) const;

        INLINE StringView<T> afterFirstOrFull(const StringView<T>& pattern) const;
        INLINE StringView<T> afterFirstNoCaseOrFull(const StringView<T>& pattern) const;

        INLINE StringView<T> beforeFirstOrFull(const StringView<T>& pattern) const;
        INLINE StringView<T> beforeFirstNoCaseOrFull(const StringView<T>& pattern) const;

        INLINE StringView<T> afterLastOrFull(const StringView<T>& pattern) const;
        INLINE StringView<T> afterLastNoCaseOrFull(const StringView<T>& pattern) const;

        INLINE StringView<T> beforeLastOrFull(const StringView<T>& pattern) const;
        INLINE StringView<T> beforeLastNoCaseOrFull(const StringView<T>& pattern) const;

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
        INLINE bool matchString(const StringView<T>& pattern) const;
        INLINE bool matchStringNoCase(const StringView<T>& pattern) const;

        // match a pattern or sub string
        INLINE bool matchPattern(const StringView<T>& pattern) const;
        INLINE bool matchPatternNoCase(const StringView<T>& pattern) const;

        //-----------------------------------------------------------------------------

        // compute hash of the string
        INLINE static uint32_t CalcHash(StringView<T> txt);

        // compute 32-bit crc
        INLINE uint32_t calcCRC32(uint32_t crc = CRC32Init) const;

        // compute 64-bit crr
        INLINE uint64_t calcCRC64(uint64_t crc = CRC64Init) const;

        //-----------------------------------------------------------------------------

        // create a memory buffer, NOTE: no null termination
        INLINE Buffer toBuffer() const;

        // create a memory buffer, NOTE: no null termination
        INLINE Buffer toBufferWithZero() const;

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
        INLINE PathEater(StringView<T> str, bool pureDirPath = false)
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
        INLINE StringView<char> restOfThePath() const
        {
            return StringView<char>(m_cur, m_end);
        }

        /// reset parsing
        INLINE void reset()
        {
            m_cur = m_begin;
        }

        /// eat next part
        INLINE StringView<char> eatDirectoryName()
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
                    return StringView<char>(start, length);
                }

                // go to next char
                m_cur += 1;
            }

            // in the pure path the last entry is a dir as well even without the "/"
            if (m_pureDirPath && m_cur > start && !*m_cur)
                return StringView<char>(start, m_end);

            // not a directory
            m_cur = start;
            return StringView<char>();
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

} // base
