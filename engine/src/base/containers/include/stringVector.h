  /***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

#include "stringView.h"

namespace base
{

    //-----------------------------------------------------------------------------

    /// Simple string buffer based on container
    /// NOTE: it's either Ansi or Unicode
    template< typename T >
    class StringVector
	{
    public:
        INLINE StringVector();
        INLINE StringVector(const StringVector& other);
        INLINE StringVector(StringVector&& other);

        INLINE explicit StringVector(const char* str, uint32_t length = INDEX_MAX); // when converting to wide char we assume it's UTF8
        INLINE explicit StringVector(const wchar_t* str, uint32_t length = INDEX_MAX);
        INLINE explicit StringVector(StringView<char> view); // when converting to wide char we assume it's UTF8
        INLINE explicit StringVector(const StringView<wchar_t>& view);

        INLINE ~StringVector() = default;

        //--------------------------

        INLINE bool empty() const;

        INLINE uint32_t length() const;

        INLINE StringView<T> view() const;

        INLINE operator StringView<T>() const;

        INLINE const T* c_str() const;

        INLINE StringBuf ansi_str() const;

        INLINE T* c_str();

        INLINE void print(IFormatStream& f) const;

        //--------------------------

        INLINE StringVector<T>& reserve(uint32_t size);

        INLINE StringVector<T>& clear();

        INLINE StringVector<T>& append(const StringVector<T>& str);

        INLINE StringVector<T>& append(StringView<char> str);

        INLINE StringVector<T>& append(const StringView<wchar_t>& str);

        //--------------------------

        INLINE StringVector<T>& operator=(const StringVector<T>& other);
        INLINE StringVector<T>& operator=(StringVector<T>&& other);
        INLINE StringVector<T>& operator=(StringView<char> str);
        INLINE StringVector<T>& operator=(const StringView<wchar_t>& str);
        INLINE StringVector<T>& operator=(const char* str);
        INLINE StringVector<T>& operator=(const wchar_t* str);

        INLINE bool operator==(const StringView<T>& str) const;
        INLINE bool operator!=(const StringView<T>& other) const;
        INLINE bool operator<(const StringView<T>& other) const;
        INLINE bool operator<=(const StringView<T>& other) const;
        INLINE bool operator>(const StringView<T>& other) const;
        INLINE bool operator>=(const StringView<T>& other) const;

        //-----------------------------------------------------------------------------

        INLINE StringVector<T>& operator+=(const StringVector<T>& other);
        INLINE StringVector<T>& operator+=(const char* other);
        INLINE StringVector<T>& operator+=(const wchar_t* other);
        INLINE StringVector<T>& operator+=(StringView<char> other);
        INLINE StringVector<T>& operator+=(const StringView<wchar_t>& other);

        //-----------------------------------------------------------------------------

        INLINE StringVector<T> operator+(const StringVector<T>& other) const;
        INLINE StringVector<T> operator+(const char* other) const;
        INLINE StringVector<T> operator+(const wchar_t* other) const;
        INLINE StringVector<T> operator+(StringView<char> other) const;
        INLINE StringVector<T> operator+(const StringView<wchar_t>& other) const;

        //-----------------------------------------------------------------------------

        INLINE int compareWith(const StringView<T>& other) const;
        INLINE int compareWithNoCase(const StringView<T>& other) const;

        //-----------------------------------------------------------------------------

        INLINE StringVector<T> leftPart(uint32_t count) const;
        INLINE StringVector<T> rightPart(uint32_t count) const;
        INLINE StringVector<T> subString(uint32_t first, uint32_t count = (uint32_t)-1) const;

        INLINE void split(uint32_t index, StringVector<T>& outLeft, StringVector<T>& outRight) const;
        INLINE bool splitAt(const StringView<T>& str, StringVector<T>& outLeft, StringVector<T>& outRight) const;

        //-----------------------------------------------------------------------------

        INLINE int findStr(const StringView<T>& pattern, int firstPosition = 0) const;
        INLINE int findStrRev(const StringView<T>& pattern, int firstPosition = 0) const;
        INLINE int findStrNoCase(const StringView<T>& pattern, int firstPosition = 0) const;
        INLINE int findStrRevNoCase(const StringView<T>& pattern, int firstPosition = 0) const;

        INLINE int findFirstChar(T ch) const;
        INLINE int findLastChar(T ch) const;

        INLINE bool beginsWith(const StringView<T>& pattern) const;
        INLINE bool beginsWithNoCase(const StringView<T>& pattern) const;

        INLINE bool endsWith(const StringView<T>& pattern) const;
        INLINE bool endsWithNoCase(const StringView<T>& pattern) const;

        INLINE StringVector<T> stringAfterFirst(const StringView<T>& pattern) const;
        INLINE StringVector<T> stringBeforeFirst(const StringView<T>& pattern) const;
        INLINE StringVector<T> stringAfterLast(const StringView<T>& pattern) const;
        INLINE StringVector<T> stringBeforeLast(const StringView<T>& pattern) const;

        INLINE StringVector<T> stringAfterFirstNoCase(const StringView<T>& pattern) const;
        INLINE StringVector<T> stringBeforeFirstNoCase(const StringView<T>& pattern) const;
        INLINE StringVector<T> stringAfterLastNoCase(const StringView<T>& pattern) const;
        INLINE StringVector<T> stringBeforeLastNoCase(const StringView<T>& pattern) const;

        //-----------------------------------------------------------------------------

        INLINE StringVector<T> toLower() const;
        INLINE StringVector<T> toUpper() const;

        //-----------------------------------------------------------------------------

        INLINE ArrayIterator<T> begin();
        INLINE ArrayIterator<T> end();

        INLINE ConstArrayIterator<T> begin() const;
        INLINE ConstArrayIterator<T> end() const;

        //-----------------------------------------------------------------------------

    private:
        Array<T> m_buf;

        INLINE void zeroCheck();
    };

    //--

    using UTF16StringBuf = StringVector<wchar_t>;

} // base
