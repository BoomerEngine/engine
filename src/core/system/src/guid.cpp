/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: utils #]
***/

#include "build.h"
#include "guid.h"

#ifdef PLATFORM_WINDOWS
#include <combaseapi.h>
#elif defined(PLATFORM_POSIX)
#endif

BEGIN_BOOMER_NAMESPACE()

//--

void GUID::print(IFormatStream& f) const
{
    const auto* bytes = (const uint8_t*)data();

    // print in cannonical form: {123e4567 - e89b - 12d3 - a456 - 426652340000}

    f.append("{");
    f.appendHexNumber(bytes + 0, 4);
    f.append("-");
    f.appendHexNumber(bytes + 4, 2);
    f.append("-");
    f.appendHexNumber(bytes + 6, 2);
    f.append("-");
    f.appendHexNumber(bytes + 8, 2);
    f.append("-");
    f.appendHexNumber(bytes + 10, 6);
    f.append("}");
}

//--

uint32_t GUID::CalcHash(const GUID& guid)
{
    uint32_t ret = guid.m_words[0];
    ret = (ret * 17) + guid.m_words[1];
    ret = (ret * 17) + guid.m_words[2];
    ret = (ret * 17) + guid.m_words[3];
    return ret;
}

//--

GUID GUID::Create()
{
    GUID ret;

#ifdef PLATFORM_WINDOWS
    ::GUID data;
    ::CoCreateGuid(&data);
    ret.m_words[0] = ((const uint32_t*)&data)[0];
    ret.m_words[1] = ((const uint32_t*)&data)[1];
    ret.m_words[2] = ((const uint32_t*)&data)[2];
    ret.m_words[3] = ((const uint32_t*)&data)[3];
#else
    ret.m_words[0] = (rand() << 16) ^ rand();
    ret.m_words[1] = (rand() << 16) ^ rand();
    ret.m_words[2] = (rand() << 16) ^ rand();
    ret.m_words[3] = (rand() << 16) ^ rand();
#endif

    return ret;
}

//--

static bool GetHexDigit(char ch, uint8_t& outValue)
{
    switch (ch)
    {
    case '0': outValue = 0; return true;
    case '1': outValue = 1; return true;
    case '2': outValue = 2; return true;
    case '3': outValue = 3; return true;
    case '4': outValue = 4; return true;
    case '5': outValue = 5; return true;
    case '6': outValue = 6; return true;
    case '7': outValue = 7; return true;
    case '8': outValue = 8; return true;
    case '9': outValue = 9; return true;
    case 'A': outValue = 10; return true;
    case 'B': outValue = 11; return true;
    case 'C': outValue = 12; return true;
    case 'D': outValue = 13; return true;
    case 'E': outValue = 14; return true;
    case 'F': outValue = 15; return true;
    case 'a': outValue = 10; return true;
    case 'b': outValue = 11; return true;
    case 'c': outValue = 12; return true;
    case 'd': outValue = 13; return true;
    case 'e': outValue = 14; return true;
    case 'f': outValue = 15; return true;
    }

    return false;
}

static bool GetHexByte(const char* txt, uint8_t& outValue)
{
    uint8_t a = 0, b = 0;
    if (!GetHexDigit(txt[0], a))
        return false;
    if (!GetHexDigit(txt[1], b))
        return false;

    outValue = (a << 4) | b;
    return true;
}

static bool ParseHex(const char*& txt, const char* end, uint32_t numBytes, uint8_t*& writeBytes)
{
    if (txt + (numBytes * 2) > end)
        return false;

    for (uint32_t i = 0; i < numBytes; ++i)
    {
        uint8_t byte = 0;
        if (!GetHexByte(txt, byte))
            return false;
        txt += 2;

        writeBytes[(numBytes - 1) - i] = byte;
    }

    writeBytes += numBytes;
    return true;
}

static bool ParseChar(const char*& txt, const char* end, char ch)
{
    while (txt < end)
    {
        if (*txt > ' ')
            break;
        ++txt;
    }

    if (txt >= end || *txt != ch)
        return false;

    ++txt;
    return true;
}

bool GUID::Parse(const char* txt, uint32_t length, GUID& outValue)
{
    // {123e4567-e89b-12d3-a456-426652340000}
    if (length < 34)
        return false;

    // skip white space
    const auto* end = txt + length;

    // expect the '{'
    if (!ParseChar(txt, end, '{'))
        return false;

    // parse guid parts
    uint8_t bytes[16];
    auto* writePtr = bytes;
    if (!ParseHex(txt, end, 4, writePtr))
        return false;
    if (!ParseChar(txt, end, '-'))
        return false;
    if (!ParseHex(txt, end, 2, writePtr))
        return false;
    if (!ParseChar(txt, end, '-'))
        return false;
    if (!ParseHex(txt, end, 2, writePtr))
        return false;
    if (!ParseChar(txt, end, '-'))
        return false;
    if (!ParseHex(txt, end, 2, writePtr))
        return false;
    if (!ParseChar(txt, end, '-'))
        return false;
    if (!ParseHex(txt, end, 6, writePtr))
        return false;

    // parse the end '{}'
    if (!ParseChar(txt, end, '}'))
        return false;

    // assemble value
    memcpy(outValue.m_words, bytes, sizeof(bytes));
    return true;
}

//--

END_BOOMER_NAMESPACE()
