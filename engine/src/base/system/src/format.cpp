/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: platform\output #]
***/

#include "build.h"
#include "format.h"

#include <stdarg.h>

namespace base
{
    ///---

    namespace prv
    {

        static void ReverseInplace(char* ptr)
        {
            if (*ptr)
            {
                auto end = ptr + strlen(ptr) - 1;
                while (ptr < end)std::swap(*ptr++, *end--);
            }
        }

        uint8_t ConvertChar(char* dest, uint32_t ch)
        {
            if (ch < 0x80)
            {
                dest[0] = (char)ch;
                return 1;
            }
            else if (ch < 0x800)
            {
                dest[0] = (char)((ch >> 6) | 0xC0);
                dest[1] = (char)((ch & 0x3F) | 0x80);
                return 2;
            }
            else if (ch < 0x10000)
            {
                dest[0] = (char)((ch >> 12) | 0xE0);
                dest[1] = (char)(((ch >> 6) & 0x3F) | 0x80);
                dest[2] = (char)((ch & 0x3F) | 0x80);
                return 3;
            }
            else if (ch < 0x110000)
            {
                dest[0] = (char)((ch >> 18) | 0xF0);
                dest[1] = (char)(((ch >> 12) & 0x3F) | 0x80);
                dest[2] = (char)(((ch >> 6) & 0x3F) | 0x80);
                dest[3] = (char)((ch & 0x3F) | 0x80);
                return 4;
            }
            return 0;
        }

    } // prv

    ///---

    IFormatStream::~IFormatStream()
    {}

    IFormatStream& IFormatStream::appendNumber(char val)
    {
        return appendNumber((int64_t)val);
    }

    IFormatStream& IFormatStream::appendNumber(short val)
    {
        return appendNumber((int64_t)val);
    }

    IFormatStream& IFormatStream::appendNumber(int val)
    {
        return appendNumber((int64_t)val);
    }

    IFormatStream& IFormatStream::appendNumber(int64_t original)
    {
        char buf[33];

        char* ptr = buf;
        auto n = original;
        auto sign = n < 0;
        if (sign)
            n = -n;

        do
        {
            *ptr++ = n % 10 + '0';
        } while ((n /= 10) > 0);

        if (sign)
            *ptr++ = '-';
        *ptr++ = 0;

        prv::ReverseInplace(buf);
        return append(buf);
    }

    IFormatStream& IFormatStream::appendNumber(uint8_t val)
    {
        return appendNumber((uint64_t)val);
    }

    IFormatStream& IFormatStream::appendNumber(uint16_t val)
    {
        return appendNumber((uint64_t)val);
    }

    IFormatStream& IFormatStream::appendNumber(uint32_t val)
    {
        return appendNumber((uint64_t)val);
    }

    IFormatStream& IFormatStream::appendNumber(uint64_t original)
    {
        char buf[33];

        auto n = original;
        char* ptr = buf;
        do
        {
            *ptr++ = n % 10 + '0';
        } while ((n /= 10) > 0);

        *ptr++ = 0;

        prv::ReverseInplace(buf);
        return append(buf);
    }

    IFormatStream& IFormatStream::appendNumber(float val)
    {
        char buf[128];
        sprintf(buf, "%f", val);
        return append(buf);
    }

    IFormatStream& IFormatStream::appendNumber(double val)
    {
        char buf[128];
        sprintf(buf, "%f", val);
        return append(buf);
    }

    const char HexTable[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    IFormatStream& IFormatStream::appendHexNumber(const void* srcData, uint32_t size)
    {
        //append("0x");

        auto data  = (const uint8_t*)srcData + size-1;
        auto end  = (const uint8_t*)srcData;

        char temp[18];

        while (data >= end)
        {
            char* write = temp;
            char* writeEnd = temp + 16;
            while (data >= end && write < writeEnd)
            {
                *write++ = HexTable[(*data >> 4) & 15];
                *write++ = HexTable[(*data) & 15];
                data -= 1;
            }
            *write++ = 0;
            append(temp);
        }

        return *this;
    }

    IFormatStream& IFormatStream::appendHexBlock(const void* srcData, uint32_t size, uint32_t blockSize)
    {
        auto data  = (const uint8_t*)srcData;
        auto end  = data + size;

        char temp[34];

        while (data < end)
        {
            char* write = temp;
            char* writeEnd = temp + std::clamp<uint32_t>(blockSize, 1, ARRAY_COUNT(temp)-2);
            while (data < end && write < writeEnd)
            {
                *write++ = HexTable[(*data >> 4) & 15];
                *write++ = HexTable[(*data) & 15];
                data += 1;
            }

            if (data < end)
                *write++ = ' ';

            *write++ = 0;
            append(temp);
        }

        return *this;
    }

    IFormatStream& IFormatStream::appendBase64(const void* data, uint32_t size)
    {
        static const char* Base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        uint32_t i = 0;
        uint8_t bytes[3];

        auto readPtr  = (const uint8_t*)data;
        auto endPtr  = readPtr + size;
        while (readPtr < endPtr)
        {
            bytes[i++] = *readPtr++;
            if (i == 3)
            {
                appendch(Base64Chars[ (bytes[0] & 0xfc) >> 2 ]);
                appendch(Base64Chars[((bytes[0] & 0x03) << 4) + ((bytes[1] & 0xf0) >> 4)]);
                appendch(Base64Chars[((bytes[1] & 0x0f) << 2) + ((bytes[2] & 0xc0) >> 6)]);
                appendch(Base64Chars[bytes[2] & 0x3f]);
                i = 0;
            }
        }

        if (i)
        {
            for (uint32_t j=i; j<3; j++)
                bytes[j] = 0;

            uint8_t chars[3];
            chars[0] = ( bytes[0] & 0xfc) >> 2;
            chars[1] = ((bytes[0] & 0x03) << 4) + ((bytes[1] & 0xf0) >> 4);
            chars[2] = ((bytes[1] & 0x0f) << 2) + ((bytes[2] & 0xc0) >> 6);

            for (uint32_t j=0; j < i + 1; j++)
                appendch(Base64Chars[chars[j]]);

            while (i++ < 3)
                appendch('=');
        }

        return *this;
    }

    IFormatStream& IFormatStream::appendPreciseNumber(double val, uint32_t numDigits)
    {
        char pattern[20];
        sprintf(pattern, "%%1.%df", numDigits);

        char buffer[128];
        sprintf(buffer, pattern, val);
        return append(buffer);
    }


    static inline bool Curl_isunreserved(unsigned char in) {
        switch (in) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
        case 'a': case 'b': case 'c': case 'd': case 'e':
        case 'f': case 'g': case 'h': case 'i': case 'j':
        case 'k': case 'l': case 'm': case 'n': case 'o':
        case 'p': case 'q': case 'r': case 's': case 't':
        case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
        case 'A': case 'B': case 'C': case 'D': case 'E':
        case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O':
        case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
        case '-': case '.': case '_': case '~':
            return true;
        default:
            break;
        }
        return false;
    }

    IFormatStream& IFormatStream::appendUrlEscaped(const char* data, uint32_t length)
    {
        if (length == INDEX_MAX) length = strlen(data);

        auto start  = data;
        auto end  = data + length;
        auto pos  = start;

        while (pos < end) {
            auto ch = *pos;

            if (Curl_isunreserved(ch)) {
                pos += 1;
                continue;
            }

            if (pos > start)
                append(start, pos - start);

            appendf("%{}", Hex(ch));

            pos += 1;
            start = pos;
        }

        if (pos > start)
            append(start, pos - start);

        return *this;
    }

    bool IsLegalChar(char ch)
    {
        if (ch >= 'A' && ch <= 'Z') return true;
        else if (ch >= 'a' && ch <= 'z') return true;
        else if (ch >= '0' && ch <= '9') return true;
        else if (ch < ' ' || ch >= 127 || ch == '(' || ch == ')' || ch == '=' || ch == ']' || ch == '[' || ch == '\"') return false;
        return true;
    }

    IFormatStream& IFormatStream::appendCEscaped(const char* data, uint32_t length, bool willBeSentInQuotes)
    {
        if (length == INDEX_MAX) length = strlen(data);

        auto start  = data;
        auto end  = data + length;
        auto pos  = start;

        while (pos < end)
        {
            auto ch = *pos;

            if (ch >= ' ' && ch <= 127 && (!willBeSentInQuotes || (ch != '\"' && ch != '\'')))
            {
                pos += 1;
                continue;
            }

            if (pos > start)
                append(start, pos - start);

            if (ch == 10)
                append("\\n");
            else if (ch == 13)
                append("\\r");
            else if (ch == 9)
                append("\\t");
            else if (ch == 0)
                append("\\0");
            else if (ch == 8)
                append("\\b");
            else if (ch == '\"')
                append("\\\"");
            else if (ch == '\'')
                append("\\\'");
            else 
                appendf("\\x{}", Hex(ch));

            pos += 1;
            start = pos;
        }

        if (pos > start)
            append(start, pos - start);

        return *this;
    }

    IFormatStream& IFormatStream::append(const wchar_t* str, uint32_t len)
    {
        // NOTE: unicode conversion is not efficent but on the other hand it's also not very common
        // TODO: preallocate all needed memory in one go
        auto end = (len == INDEX_MAX) ? (str + wcslen(str)) : (str + len);
        while (str < end)
        {
            char buf[6];
            auto len = prv::ConvertChar(buf, *str++);
            append(buf, len);
        }

        return *this;
    }

    IFormatStream& IFormatStream::appendUTF32(uint32_t ch)
    {
        char buf[7];
        auto len = prv::ConvertChar(buf, ch);
        append(buf, len);

        return *this;
    }
    
    IFormatStream& IFormatStream::appendPadding(char ch, uint32_t count)
    {
        auto maxPadding = std::min<uint32_t>(count, 64);

        char str[64];
        memset(str, ch, maxPadding);

        return append(str, maxPadding);
    }

    ///---

    class DevNullStream : public IFormatStream
    {
    public:
        virtual IFormatStream& append(const char* str, uint32_t len) { return *this; }
    };

    IFormatStream& IFormatStream::NullStream()
    {
        static DevNullStream theNullStream;
        return theNullStream;
    }

    ///---

    bool IFormatStream::consumeFormatString(const char*& pos)
    {
        auto start  = pos;
        while (*pos)
        {
            if (pos[0] == '{' && pos[1] == '}')
            {
                append(start, range_cast<uint32_t>(pos - start));
                pos += 2;
                return true;
            }
            ++pos;
        }

        append(start, range_cast<uint32_t>(pos - start));
        return false;
    }

    //---

} // base
