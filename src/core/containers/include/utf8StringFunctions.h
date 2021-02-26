/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string\utf8 #]
***/

#pragma once

/*
  BASED ON:

  Basic UTF-8 manipulation routines
  by Jeff Bezanson
  placed in the public domain Fall 2005
*/

BEGIN_BOOMER_NAMESPACE()

namespace utf8
{
    //--

    // is c the start of a utf8 sequence
    ALWAYS_INLINE static bool IsUTF8(char c)
    {
        return (c & 0xC0) != 0x80;
    }

    ///--

    // TODO: inline 

    // Convert UTF-8 data to 16-bit wide character
    // NOTE: the target buffer must be large enough, returns number of written chars
    extern CORE_CONTAINERS_API size_t ToUniChar(wchar_t* dest, size_t sz, const char* src, size_t srcsz);

    // Convert UTF-8 data to the very wide 32-bit character
    // NOTE: the target buffer must be large enough, returns number of written chars
    extern CORE_CONTAINERS_API size_t ToUniChar32(uint32_t* dest, size_t sz, const char* src, size_t srcsz);

    // Convert wide characters to UTF-8 data
    // NOTE: the target buffer must be large enough, returns number of written chars
    extern CORE_CONTAINERS_API size_t FromUniChar(char* dest, size_t sz, const wchar_t* src, size_t srcsz);

    // NOTE: the target buffer must be large enough, returns number of written chars
    extern CORE_CONTAINERS_API size_t FromUniChar32(char* dest, size_t sz, const uint32_t* src, size_t srcsz);

    // Single character to UTF-8, returns number of written chars
    extern CORE_CONTAINERS_API uint8_t ConvertChar(char* dest, uint32_t code);

    // Return next character, updating an index variable
    extern CORE_CONTAINERS_API uint32_t NextChar(const char*& ptr);

    // Return next character, updating an index variable
    extern CORE_CONTAINERS_API uint32_t NextChar(const char*& ptr, const char* endPtr);

    // Return true if we can parse a utf-8 character at given position
    extern CORE_CONTAINERS_API bool ValidChar(const char* ptr, const char* endPtr);

    // Get char code parser from utf-8 characters at given position
    extern CORE_CONTAINERS_API uint32_t GetChar(const char* ptr, const char* endPtr);

    // Count the number of characters in a UTF-8 string
    extern CORE_CONTAINERS_API size_t Length(const char* s);

    // Count memory size required to encode UTF-8 string from uint32_t buffer
    // NOTE: does not include the terminating zero
    extern CORE_CONTAINERS_API size_t CalcSizeRequired(const uint32_t* s, size_t maxLength = MAX_SIZE_T);

    // Count memory size required to encode UTF-8 string from wchar_t buffer
    // NOTE: does not include the terminating zero
    extern CORE_CONTAINERS_API size_t CalcSizeRequired(const wchar_t* s, size_t maxLength = MAX_SIZE_T);

    ///--

    // "Iterate" over decoded UTF8 chars, should be used when we want to interpret the string content as actual text
    class CharIterator
    {
    public:
        INLINE CharIterator() {};
        INLINE CharIterator(const char* start, uint32_t length=UINT_MAX);
        INLINE CharIterator(const char* start, const char* end);
        INLINE CharIterator(StringView txt);
        INLINE CharIterator(const CharIterator& other) = default;
        INLINE CharIterator& operator=(const CharIterator& other) = default;
        INLINE ~CharIterator() = default;

        INLINE operator bool() const;
            
        INLINE uint32_t operator*() const;

        INLINE void operator++();
        INLINE void operator++(int);

    private:
        const char* m_pos = nullptr;
        const char* m_end = nullptr;
    };

    ///--

} // utf8

END_BOOMER_NAMESPACE()

#include "utf8StringFunctions.inl"