  /***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

#include "stringView.h"

BEGIN_BOOMER_NAMESPACE(base)

//-----------------------------------------------------------------------------

/// Simple string buffer based on container
/// NOTE: it's either Ansi or Unicode
template< typename T >
class BaseStringVector
{
public:
    INLINE BaseStringVector();
    INLINE BaseStringVector(const BaseStringVector& other);
    INLINE BaseStringVector(BaseStringVector&& other);

    INLINE explicit BaseStringVector(const char* str, uint32_t length = INDEX_MAX); // when converting to wide char we assume it's UTF8
    INLINE explicit BaseStringVector(const wchar_t* str, uint32_t length = INDEX_MAX);
    INLINE explicit BaseStringVector(BaseStringView<char> view); // when converting to wide char we assume it's UTF8
    INLINE explicit BaseStringVector(const BaseStringView<wchar_t>& view);

    INLINE ~BaseStringVector() = default;

    //--------------------------

    INLINE bool empty() const;

    INLINE uint32_t length() const;

    INLINE BaseStringView<T> view() const;

    INLINE operator BaseStringView<T>() const;

    INLINE const T* c_str() const;

    INLINE StringBuf ansi_str() const;

    INLINE T* c_str_writable();

    INLINE void print(IFormatStream& f) const;

    //--------------------------

    INLINE BaseStringVector<T>& reserve(uint32_t size);

    INLINE BaseStringVector<T>& clear();

    INLINE BaseStringVector<T>& append(const BaseStringVector<T>& str);

    INLINE BaseStringVector<T>& append(BaseStringView<char> str);

    INLINE BaseStringVector<T>& append(const BaseStringView<wchar_t>& str);

    //--------------------------

    INLINE BaseStringVector<T>& operator=(const BaseStringVector<T>& other);
    INLINE BaseStringVector<T>& operator=(BaseStringVector<T>&& other);
    INLINE BaseStringVector<T>& operator=(BaseStringView<char> str);
    INLINE BaseStringVector<T>& operator=(const BaseStringView<wchar_t>& str);
    INLINE BaseStringVector<T>& operator=(const char* str);
    INLINE BaseStringVector<T>& operator=(const wchar_t* str);

    INLINE bool operator==(const BaseStringView<T>& str) const;
    INLINE bool operator!=(const BaseStringView<T>& other) const;
    INLINE bool operator<(const BaseStringView<T>& other) const;
    INLINE bool operator<=(const BaseStringView<T>& other) const;
    INLINE bool operator>(const BaseStringView<T>& other) const;
    INLINE bool operator>=(const BaseStringView<T>& other) const;

    //-----------------------------------------------------------------------------

    INLINE BaseStringVector<T>& operator+=(const BaseStringVector<T>& other);
    INLINE BaseStringVector<T>& operator+=(const char* other);
    INLINE BaseStringVector<T>& operator+=(const wchar_t* other);
    INLINE BaseStringVector<T>& operator+=(BaseStringView<char> other);
    INLINE BaseStringVector<T>& operator+=(const BaseStringView<wchar_t>& other);

    //-----------------------------------------------------------------------------

    INLINE BaseStringVector<T> operator+(const BaseStringVector<T>& other) const;
    INLINE BaseStringVector<T> operator+(const char* other) const;
    INLINE BaseStringVector<T> operator+(const wchar_t* other) const;
    INLINE BaseStringVector<T> operator+(BaseStringView<char> other) const;
    INLINE BaseStringVector<T> operator+(const BaseStringView<wchar_t>& other) const;

    //-----------------------------------------------------------------------------

    INLINE int compareWith(const BaseStringView<T>& other) const;
    INLINE int compareWithNoCase(const BaseStringView<T>& other) const;

    //-----------------------------------------------------------------------------

    INLINE BaseStringVector<T> leftPart(uint32_t count) const;
    INLINE BaseStringVector<T> rightPart(uint32_t count) const;
    INLINE BaseStringVector<T> subString(uint32_t first, uint32_t count = (uint32_t)-1) const;

    INLINE void split(uint32_t index, BaseStringVector<T>& outLeft, BaseStringVector<T>& outRight) const;
    INLINE bool splitAt(const BaseStringView<T>& str, BaseStringVector<T>& outLeft, BaseStringVector<T>& outRight) const;

    //-----------------------------------------------------------------------------

    INLINE int findStr(const BaseStringView<T>& pattern, int firstPosition = 0) const;
    INLINE int findStrRev(const BaseStringView<T>& pattern, int firstPosition = 0) const;
    INLINE int findStrNoCase(const BaseStringView<T>& pattern, int firstPosition = 0) const;
    INLINE int findStrRevNoCase(const BaseStringView<T>& pattern, int firstPosition = 0) const;

    INLINE int findFirstChar(T ch) const;
    INLINE int findLastChar(T ch) const;

    INLINE bool beginsWith(const BaseStringView<T>& pattern) const;
    INLINE bool beginsWithNoCase(const BaseStringView<T>& pattern) const;

    INLINE bool endsWith(const BaseStringView<T>& pattern) const;
    INLINE bool endsWithNoCase(const BaseStringView<T>& pattern) const;

    INLINE BaseStringVector<T> stringAfterFirst(const BaseStringView<T>& pattern) const;
    INLINE BaseStringVector<T> stringBeforeFirst(const BaseStringView<T>& pattern) const;
    INLINE BaseStringVector<T> stringAfterLast(const BaseStringView<T>& pattern) const;
    INLINE BaseStringVector<T> stringBeforeLast(const BaseStringView<T>& pattern) const;

    INLINE BaseStringVector<T> stringAfterFirstNoCase(const BaseStringView<T>& pattern) const;
    INLINE BaseStringVector<T> stringBeforeFirstNoCase(const BaseStringView<T>& pattern) const;
    INLINE BaseStringVector<T> stringAfterLastNoCase(const BaseStringView<T>& pattern) const;
    INLINE BaseStringVector<T> stringBeforeLastNoCase(const BaseStringView<T>& pattern) const;

    //-----------------------------------------------------------------------------

    INLINE BaseStringVector<T> toLower() const;
    INLINE BaseStringVector<T> toUpper() const;

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

using StringVector = BaseStringVector<char>;
using UTF16StringVector = BaseStringVector<wchar_t>;

//--

END_BOOMER_NAMESPACE(base)
