/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#include "build.h"
#include "stringParser.h"
#include "stringBuilder.h"
#include "stringID.h"
#include "utf8StringFunctions.h"

BEGIN_BOOMER_NAMESPACE()

namespace prv
{
    ///---

    template< typename T >
    INLINE static bool IsFloatNum(uint32_t index, T ch)
    {
        if (ch == '.') return true;
        if (ch >= '0' && ch <= '9') return true;
        if (ch == 'f' && (index > 0)) return true;
        if ((ch == '+' || ch == '-') && (index == 0)) return true;
        return false;
    }

    template< typename T >
    INLINE static bool IsAlphaNum(T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if (ch >= 'A' && ch <= 'Z') return true;
        if (ch >= 'a' && ch <= 'z') return true;
        if (ch == '_') return true;
        return false;
    }

    template< typename T >
    INLINE static bool IsStringChar(T ch, const T* additionalDelims)
    {
        if (ch <= ' ') return false;
        if (ch == '\"' || ch == '\'') return false;

        if (strchr(additionalDelims, ch))
            return false;

        return true;
    }

    template< typename T >
    INLINE static bool IsIntNum(uint32_t index, T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if ((ch == '+' || ch == '-') && (index == 0)) return true;
        return false;
    }

    template< typename T >
    INLINE static bool IsHex(T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if (ch >= 'A' && ch <= 'F') return true;
        if (ch >= 'a' && ch <= 'f') return true;
        return false;
    }

    template< typename T >
    INLINE uint64_t GetHexValue(T ch)
    {
        switch (ch)
        {
            case '0': return 0;
            case '1': return 1;
            case '2': return 2;
            case '3': return 3;
            case '4': return 4;
            case '5': return 5;
            case '6': return 6;
            case '7': return 7;
            case '8': return 8;
            case '9': return 9;
            case 'a': return 10;
            case 'A': return 10;
            case 'b': return 11;
            case 'B': return 11;
            case 'c': return 12;
            case 'C': return 12;
            case 'd': return 13;
            case 'D': return 13;
            case 'e': return 14;
            case 'E': return 14;
            case 'f': return 15;
            case 'F': return 15;
        }

        return 0;
    }

} // prv

//-----

StringParser::StringParser()
    : m_start(nullptr)
    , m_cur(nullptr)
    , m_end(nullptr)
    , m_line(1)
{}

StringParser::StringParser(StringView view)
    : m_start(view.data())
    , m_cur(view.data())
    , m_end(view.data() + view.length())
    , m_line(1)
{}

StringParser::StringParser(const void* data, uint32_t size)
    : m_start((const char*)data)
    , m_line(1)
{
    m_cur = m_start;
    m_end = m_start + size;
}

StringParser::StringParser(const StringParser& other)
{
    m_start = other.m_start;
    m_cur = other.m_cur;
    m_end = other.m_end;
    m_line = other.m_line;
}

StringParser& StringParser::operator=(const StringParser& other)
{
    if (this != &other)
    {
        m_start = other.m_start;
        m_cur = other.m_cur;
        m_end = other.m_end;
        m_line = other.m_line;
    }

    return *this;
}

void StringParser::clear()
{
    m_start = nullptr;
    m_cur = nullptr;
    m_end = nullptr;
    m_line = 1;
}

void StringParser::rewind()
{
    m_cur = m_start;
    m_line = 1;
}

bool StringParser::parseWhitespaces()
{
    while (m_cur < m_end && *m_cur <= ' ')
    {
        if (*m_cur == '\n')
            m_line += 1;
        m_cur++;
    }

    return m_cur < m_end;
}

bool StringParser::parseChar(uint32_t& outChar)
{
    auto* cur = m_cur;
    const auto ch = utf8::NextChar(cur, m_end);
    if (ch != 0)
    {
        outChar = ch;
        m_cur = cur;
        return true;
    }

    return false;
}

bool StringParser::parseTillTheEndOfTheLine(StringView* outIdent /*= nullptr*/)
{
    const char* firstNonEmptyChar = nullptr;
    const char* lastNonEmptyChar = nullptr;

    while (m_cur < m_end)
    {
        if (*m_cur > ' ')
        {
            if (!firstNonEmptyChar)
                firstNonEmptyChar = m_cur;

            lastNonEmptyChar = m_cur;
        }

        if (*m_cur++ == '\n')
            break;
    }

    if (outIdent)
    {
        if (lastNonEmptyChar != nullptr && m_cur < m_end)
            *outIdent = StringView(firstNonEmptyChar, lastNonEmptyChar+1);
        else
            *outIdent = StringView();
    }

    return m_cur < m_end;
}

bool StringParser::parseLine(StringView& outValue, const char* additionalDelims/* = ""*/, bool eatLeadingWhitespaces/*=true*/)
{
    // eat initial whitespaces on teh line
    while (m_cur < m_end)
    {
        if (*m_cur == '\n')
        {
            m_line += 1;
            m_cur++;
            return false;
        }

        if (*m_cur > ' ' || !eatLeadingWhitespaces)
            break;

        ++m_cur;
    }

    auto startPos  = m_cur;
    auto startLine = m_line;

    while (m_cur < m_end)
    {
        if (*m_cur == '\n')
        {
            outValue = StringView(startPos, m_cur);
            m_line += 1;
            m_cur++;
            return true;
        }

        if (strchr(additionalDelims, *m_cur))
        {
            outValue = StringView(startPos, m_cur);
            m_cur++;
            return true;
        }

        ++m_cur;
    }

    if (startPos == m_cur)
        return false;

    outValue = StringView(startPos, m_cur);
    return true;
}

bool StringParser::parseString(StringView& outValue, const char* additionalDelims)
{
    if (!parseWhitespaces())
        return false;

    auto startPos  = m_cur;
    auto startLine = m_line;

    if (*m_cur == '\"' || *m_cur == '\'')
    {
        auto quote = *m_cur++;
        auto stringStart = m_cur;
        while (m_cur < m_end && *m_cur != quote)
            if (*m_cur++ == '\n')
                m_line += 1;

        if (m_cur >= m_end)
        {
            m_cur = startPos;
            m_line = startLine;
            return false;
        }

        outValue = StringView(stringStart, m_cur);
        m_cur += 1;

        return true;
    }
    else
    {
        while (m_cur < m_end && prv::IsStringChar(*m_cur, additionalDelims))
            m_cur += 1;

        outValue = StringView(startPos, m_cur);
        return true;
    }
}

bool StringParser::parseIdentifier(StringView& outIdent)
{
    if (!parseWhitespaces())
        return false;

    if (!(*m_cur == '_' || *m_cur == ':' || std::iswalpha(*m_cur)))
        return false;

    auto identStart = m_cur;
    while (m_cur < m_end && (*m_cur == '_' || *m_cur == ':' || std::iswalnum(*m_cur)))
        m_cur += 1;

    ASSERT(m_cur > identStart);
    outIdent = StringView(identStart, m_cur);
    return true;
}

bool StringParser::parseIdentifier(StringID& outName)
{
    StringView ident;
    if (!parseIdentifier(ident))
        return false;

    outName = StringID(ident);
    return true;
}

bool StringParser::testKeyword(StringView keyword) const
{
    auto cur  = m_cur;
    while (cur < m_end && *cur <= ' ')
        ++cur;

    auto keyLength = keyword.length();
    for (uint32_t i=0; i<keyLength; ++i)
    {
        if (cur >= m_end || *cur != keyword.data()[i])
            return false;

        cur += 1;
    }

    return true;
}

bool StringParser::parseKeyword(StringView keyword)
{
    if (!parseWhitespaces())
        return false;

    auto keyStart = m_cur;
    auto keyLength = keyword.length();
    for (uint32_t i=0; i<keyLength; ++i)
    {
        if (m_cur >= m_end || *m_cur != keyword.data()[i])
        {
            m_cur = keyStart;
            return false;
        }

        m_cur += 1;
    }

    return true;
}

bool StringParser::parseHex(uint64_t& value, uint32_t maxLength /*= 0*/, uint32_t* outValueLength/*= nullptr*/)
{
    if (!parseWhitespaces())
        return false;

    const char* original = m_cur;
    const char* maxEnd = maxLength ? (m_cur + maxLength) : m_end;
    uint64_t ret = 0;
    while (m_cur < m_end && prv::IsHex(*m_cur) && (m_cur < maxEnd))
    {
        ret = (ret << 4) | prv::GetHexValue(*m_cur);
        m_cur += 1;
    }

    if (original == m_cur)
        return false;

    value = ret;
    if (outValueLength)
        *outValueLength = (uint32_t)(m_cur - original);
    return true;
}

bool StringParser::parseBoolean(bool& value)
{
    if (parseKeyword("true"))
    {
        value = true;
        return true;
    }
    else if (parseKeyword("false"))
    {
        value = false;
        return true;
    }

    int64_t numericValue = 0;
    if (parseInt64(numericValue))
    {
        value = (numericValue != 0);
        return true;
    }

    return false;
}

bool StringParser::parseFloat(float& value)
{
    double doubleValue;
    if (parseDouble(doubleValue))
    {
        value = (float)doubleValue;
        return true;
    }

    return false;
}

bool StringParser::parseDouble(double& value)
{
    if (!parseWhitespaces())
        return false;

    auto originalPos = m_cur;
    if (*m_cur == '-' || *m_cur == '+')
    {
        m_cur += 1;
    }

    uint32_t numChars = 0;
    while (m_cur < m_end && prv::IsFloatNum(numChars, *m_cur))
    {
        ++numChars;
        m_cur += 1;
    }

    if (numChars)
    {
        if (MatchResult::OK == StringView(originalPos, m_cur).match(value))
            return true;
    }

    m_cur = originalPos;
    return false;
}

bool StringParser::parseInt8(char& value)
{
    auto start  = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<char>::min() || bigVal > std::numeric_limits<char>::max())
    {
        m_cur = start;
        return false;
    }

    value = (char)bigVal;
    return true;
}

    bool StringParser::parseInt16(short& value)
    {
    auto start  = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<short>::min() || bigVal > std::numeric_limits<short>::max())
    {
        m_cur = start;
        return false;
    }

    value = (short)bigVal;
    return true;
}

bool StringParser::parseInt32(int& value)
{
    auto start  = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<int>::min() || bigVal > std::numeric_limits<int>::max())
    {
        m_cur = start;
        return false;
    }

    value = (int)bigVal;
    return true;
}

bool StringParser::parseInt64(int64_t& value)
{
    if (!parseWhitespaces())
        return false;

    auto originalPos = m_cur;
    if (*m_cur == '-' || *m_cur == '+')
        m_cur += 1;

    uint32_t numChars = 0;
    while (m_cur < m_end && prv::IsIntNum(numChars, *m_cur))
    {
        ++numChars;
        ++m_cur;
    }

    if (numChars)
    {
        if (MatchResult::OK == StringView(originalPos, m_cur).match(value))
            return true;
    }

    m_cur = originalPos;
    return false;
}

bool StringParser::parseUint8(uint8_t& value)
{
    auto start  = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint8_t>::max())
    {
        m_cur = start;
        return false;
    }

    value = (uint8_t)bigVal;
    return true;
}

bool StringParser::parseUint16(uint16_t& value)
{
    auto start  = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint16_t>::max())
    {
        m_cur = start;
        return false;
    }

    value = (uint16_t)bigVal;
    return true;
}

bool StringParser::parseUint32(uint32_t& value)
{
    auto start  = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint32_t>::max())
    {
        m_cur = start;
        return false;
    }

    value = (uint32_t)bigVal;
    return true;
}

bool StringParser::parseUint64(uint64_t& value)
{
    if (!parseWhitespaces())
        return false;

    uint32_t numChars = 0;
    auto originalPos = m_cur;
    while (m_cur < m_end && prv::IsIntNum(numChars, *m_cur))
    {
        ++numChars;
        ++m_cur;
    }

    if (numChars)
    {
        if (MatchResult::OK == StringView(originalPos, m_cur).match(value))
            return true;
    }

    m_cur = originalPos;
    return false;
}

bool StringParser::parseTypeName(StringID& typeName)
{
    int numTypeBracket = 0;
    int numNumBracket = 0;

    const char* cur = m_cur;
    while (cur < m_end)
    {
        auto ch = *cur;

        if (ch == '<')
        {
            numTypeBracket += 1;
        }
        else if (ch == '[')
        {
            numNumBracket += 1;
        }
        else if (ch == '>')
        {
            if (numTypeBracket == 0)
                break;

            numTypeBracket -= 1;
        }
        else if (ch == ']')
        {
            if (numNumBracket == 0)
                return false;
        }
        else if (ch == ',')
        {
            if (numTypeBracket == 0 && numNumBracket == 0)
                break;
        }
        else if (!prv::IsAlphaNum(ch) && ch != ':' && ch != '.')
        {
            return false;
        }

        cur += 1;
    }

    // nothing parsed
    if (cur == m_cur)
        return false;

    // left inside the parsing
    if (numTypeBracket || numNumBracket)
        return false;

    typeName = StringID(StringView(m_cur, cur));
    m_cur = cur;
    return true;
}

void StringParser::push()
{
    auto& state = m_stateStack.emplaceBack();
    state.cur = m_cur;
    state.line = m_line;
}

void StringParser::pop()
{
    ASSERT(!m_stateStack.empty());

    auto& state = m_stateStack.back();
    m_cur = state.cur;
    m_line = state.line;

    m_stateStack.popBack();
}

END_BOOMER_NAMESPACE()


