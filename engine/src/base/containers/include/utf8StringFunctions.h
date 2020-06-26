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

namespace base
{
    namespace utf8
    {
        // is c the start of a utf8 sequence
        INLINE static bool IsUTF8(char c)
        {
            return (c & 0xC0) != 0x80;
        }

        // Convert UTF-8 data to 16-bit wide character
        // NOTE: the target buffer must be large enough, returns number of written chars
        extern BASE_CONTAINERS_API size_t ToUniChar(wchar_t* dest, size_t sz, const char* src, size_t srcsz);

        // Convert UTF-8 data to the very wide 32-bit character
        // NOTE: the target buffer must be large enough, returns number of written chars
        extern BASE_CONTAINERS_API size_t ToUniChar32(uint32_t* dest, size_t sz, const char* src, size_t srcsz);

        // Convert wide characters to UTF-8 data
        // NOTE: the target buffer must be large enough, returns number of written chars
        extern BASE_CONTAINERS_API size_t FromUniChar(char* dest, size_t sz, const wchar_t* src, size_t srcsz);

        // NOTE: the target buffer must be large enough, returns number of written chars
        extern BASE_CONTAINERS_API size_t FromUniChar32(char* dest, size_t sz, const uint32_t* src, size_t srcsz);

        // Single character to UTF-8, returns number of written chars
        extern BASE_CONTAINERS_API uint8_t ConvertChar(char* dest, uint32_t code);

        // Return next character, updating an index variable
        extern BASE_CONTAINERS_API uint32_t NextChar(const char*& ptr);

        // Return next character, updating an index variable
        extern BASE_CONTAINERS_API uint32_t NextChar(const char*& ptr, const char* endPtr);

        // Count the number of characters in a UTF-8 string
        extern BASE_CONTAINERS_API size_t Length(const char* s);

        // Count memory size required to encode UTF-8 string from uint32_t buffer
        // NOTE: does not include the terminating zero
        extern BASE_CONTAINERS_API size_t CalcSizeRequired(const uint32_t* s, size_t maxLength = MAX_SIZE_T);

        // Count memory size required to encode UTF-8 string from wchar_t buffer
        // NOTE: does not include the terminating zero
        extern BASE_CONTAINERS_API size_t CalcSizeRequired(const wchar_t* s, size_t maxLength = MAX_SIZE_T);

    } // utf8
} // base