#include "common.h"
#include "utils.h"
#include <cwctype>
#include <fstream>
#include <sstream>

//--

namespace prv
{
    ///---

    template< typename T >
    inline static bool IsFloatNum(uint32_t index, T ch)
    {
        if (ch == '.') return true;
        if (ch >= '0' && ch <= '9') return true;
        if (ch == 'f' && (index > 0)) return true;
        if ((ch == '+' || ch == '-') && (index == 0)) return true;
        return false;
    }

    template< typename T >
    inline static bool IsAlphaNum(T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if (ch >= 'A' && ch <= 'Z') return true;
        if (ch >= 'a' && ch <= 'z') return true;
        if (ch == '_') return true;
        return false;
    }

    template< typename T >
    inline static bool IsStringChar(T ch, const T* additionalDelims)
    {
        if (ch <= ' ') return false;
        if (ch == '\"' || ch == '\'') return false;

        if (strchr(additionalDelims, ch))
            return false;

        return true;
    }

    template< typename T >
    inline static bool IsIntNum(uint32_t index, T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if ((ch == '+' || ch == '-') && (index == 0)) return true;
        return false;
    }

    template< typename T >
    inline static bool IsHex(T ch)
    {
        if (ch >= '0' && ch <= '9') return true;
        if (ch >= 'A' && ch <= 'F') return true;
        if (ch >= 'a' && ch <= 'f') return true;
        return false;
    }

    template< typename T >
    inline uint64_t GetHexValue(T ch)
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

    static const uint32_t OffsetsFromUTF8[6] =
    {
        0x00000000UL,
        0x00003080UL,
        0x000E2080UL,
        0x03C82080UL,
        0xFA082080UL,
        0x82082080UL
    };

    static const uint8_t TrailingBytesForUTF8[256] =
    {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
        2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
        3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
    };

    inline static bool IsUTF8(char c)
    {
        return (c & 0xC0) != 0x80;
    }

    uint32_t NextChar(const char*& ptr, const char* end)
    {
        if (ptr >= end)
            return 0;

        uint32_t ch = 0;
        size_t sz = 0;
        do
        {
            ch <<= 6;
            ch += (uint8_t)*ptr++;
            sz++;
        }
        while (ptr < end && !IsUTF8(*ptr));
        if (sz > 1)
            ch -= OffsetsFromUTF8[sz - 1];
        return ch;
    }

    //--

    static const char Digits[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E','F' };

    static inline bool GetNumberValueForDigit(char ch, uint32_t base, uint8_t& outDigit)
    {
        for (uint8_t i = 0; i < base; ++i)
        {
            if (Digits[i] == ch)
            {
                outDigit = i;
                return true;
            }
        }

        return false;
    }

    template<typename T>
    static inline bool CheckNumericalOverflow(T val, T valueToAdd)
    {
        if (valueToAdd > 0)
        {
            auto left = std::numeric_limits<T>::max() - val;
            return valueToAdd > left;
        }
        else if (valueToAdd < 0)
        {
            auto left = std::numeric_limits<T>::lowest() - val;
            return valueToAdd < left;
        }

        return false;
    }

    template<typename Ch, typename T>
    inline static bool MatchInteger(const Ch* str, T& outValue, size_t strLength, uint32_t base)
    {
        static_assert(std::is_signed<T>::value || std::is_unsigned<T>::value, "Only integer types are allowed here");

        // empty strings are not valid input to this function
        if (!str || !*str)
            return false;

        // determine start and end of parsing range as well as the sign
        auto negative = (*str == '-');
        auto strStart = (*str == '+' || *str == '-') ? str + 1 : str;
        auto strEnd = str + strLength;

        // unsigned values cannot be negative :)
        if (std::is_unsigned<T>::value && negative)
            return false;

        T minValue = std::numeric_limits<T>::min();
        T maxValue = std::numeric_limits<T>::max();

        T value = 0;
        T mult = negative ? -1 : 1;

        // assemble number
        auto pos = strEnd;
        bool overflowed = false;
        while (pos > strStart)
        {
            auto ch = *(--pos);

            // if a non-zero digit is encountered we must make sure that he mult is not overflowed already
            uint8_t digitValue;
            if (!GetNumberValueForDigit((char)ch, base, digitValue))
                return false;

            // apply
            if (digitValue != 0 && overflowed)
                return false;

            // validate that we will not overflow the type
            auto valueToAdd = (T)(digitValue * mult);
            if ((valueToAdd / mult) != digitValue)
                return false;
            if (prv::CheckNumericalOverflow<T>(value, valueToAdd))
                return false;

            // accumulate
            value += valueToAdd;

            // advance to next multiplier
            T newMult = mult * 10;
            if (newMult / 10 != mult)
                overflowed = true;
            mult = newMult;
        }

        outValue = value;
        return true;
    }

    template<typename Ch>
    inline bool MatchFloat(const Ch* str, double& outValue, size_t strLength)
    {
        // empty strings are not valid input to this function
        if (!str || !*str)
            return false;

        // determine start and end of parsing range as well as the sign
        auto negative = (*str == '-');
        auto strEnd = str + strLength;
        auto strStart = (*str == '+' || *str == '-') ? str + 1 : str;

        // validate that we have a proper characters, discover the decimal point position
        auto strDecimal = strEnd; // if decimal point was not found assume it's at the end
        {
            auto pos = strStart;
            while (pos < strEnd)
            {
                auto ch = *pos++;

                if (pos == strEnd && ch == 'f')
                    break;

                if (ch == '.')
                {
                    strDecimal = pos - 1;
                }
                else
                {
                    uint8_t value = 0;
                    if (!prv::GetNumberValueForDigit((char)ch, 10, value))
                        return false;
                }
            }
        }

        // accumulate values
        double value = 0.0f;

        // TODO: this is tragic where it comes to the precision loss....
        // TODO: overflow/underflow
        {
            double mult = 1.0f;

            auto pos = strDecimal;
            while (pos > strStart)
            {
                auto ch = *(--pos);

                uint8_t digitValue = 0;
                if (!prv::GetNumberValueForDigit((char)ch, 10, digitValue))
                    return false;

                // accumulate
                value += (double)digitValue * mult;
                mult *= 10.0;
            }
        }

        // Fractional part
        if (strDecimal < strEnd)
        {
            double mult = 0.1f;

            auto pos = strDecimal + 1;
            while (pos < strEnd)
            {
                auto ch = *(pos++);

                if (pos == strEnd && ch == 'f')
                    break;

                uint8_t digitValue = 0;
                if (!prv::GetNumberValueForDigit((char)ch, 10, digitValue))
                    return false;

                // accumulate
                value += (double)digitValue * mult;
                mult /= 10.0;
            }
        }

        outValue = negative ? -value : value;
        return true;
    }

} // prv

//--



//--

Parser::Parser(std::string_view txt)
{
    m_start = txt.data();
    m_end = m_start + txt.length();
    m_cur = m_start;
}

bool Parser::testKeyword(std::string_view keyword) const
{
    auto cur = m_cur;
    while (cur < m_end && *cur <= ' ')
        ++cur;

    auto keyLength = keyword.length();
    for (uint32_t i = 0; i < keyLength; ++i)
    {
        if (cur >= m_end || *cur != keyword.data()[i])
            return false;

        cur += 1;
    }

    return true;
}

void Parser::push()
{

}

void Parser::pop()
{

}

bool Parser::parseWhitespaces()
{
    while (m_cur < m_end && *m_cur <= ' ')
    {
        if (*m_cur == '\n')
            m_line += 1;
        m_cur++;
    }

    return m_cur < m_end;
}

bool Parser::parseTillTheEndOfTheLine(std::string_view* outIdent /*= nullptr*/)
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
            *outIdent = std::string_view(firstNonEmptyChar, (lastNonEmptyChar + 1) - firstNonEmptyChar);
        else
            *outIdent = std::string_view();
    }

    return m_cur < m_end;
}

bool Parser::parseIdentifier(std::string_view& outIdent)
{
    if (!parseWhitespaces())
        return false;

    if (!(*m_cur == '_' || *m_cur == ':' || std::iswalpha(*m_cur)))
        return false;

    auto identStart = m_cur;
    while (m_cur < m_end && (*m_cur == '_' || *m_cur == ':' || std::iswalnum(*m_cur)))
        m_cur += 1;

    assert(m_cur > identStart);
    outIdent = std::string_view(identStart, m_cur - identStart);
    return true;
}

bool Parser::parseString(std::string_view& outValue, const char* additionalDelims/* = ""*/)
{
    if (!parseWhitespaces())
        return false;

    auto startPos = m_cur;
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

        outValue = std::string_view(stringStart, m_cur - stringStart);
        m_cur += 1;

        return true;
    }
    else
    {
        while (m_cur < m_end && prv::IsStringChar(*m_cur, additionalDelims))
            m_cur += 1;

        outValue = std::string_view(startPos, m_cur - startPos);
        return true;
    }
}

bool Parser::parseLine(std::string_view& outValue, const char* additionalDelims/* = ""*/, bool eatLeadingWhitespaces/*= true*/)
{
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

    auto startPos = m_cur;
    auto startLine = m_line;

    while (m_cur < m_end)
    {
        if (*m_cur == '\n')
        {
            outValue = std::string_view(startPos, m_cur - startPos);
            m_line += 1;
            m_cur++;
            return true;
        }

        if (strchr(additionalDelims, *m_cur))
        {
            outValue = std::string_view(startPos, m_cur - startPos);
            m_cur++;
            return true;
        }

        ++m_cur;
    }

    if (startPos == m_cur)
        return false;

    outValue = std::string_view(startPos, m_cur - startPos);
    return true;
}

bool Parser::parseKeyword(std::string_view keyword)
{
    if (!parseWhitespaces())
        return false;

    auto keyStart = m_cur;
    auto keyLength = keyword.length();
    for (uint32_t i = 0; i < keyLength; ++i)
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

bool Parser::parseChar(uint32_t& outChar)
{
    auto* cur = m_cur;
    const auto ch = prv::NextChar(cur, m_end);
    if (ch != 0)
    {
        outChar = ch;
        m_cur = cur;
        return true;
    }

    return false;
}

bool Parser::parseFloat(float& outValue)
{
    double doubleValue;
    if (parseDouble(doubleValue))
    {
        outValue = (float)doubleValue;
        return true;
    }

    return false;
}

bool Parser::parseDouble(double& outValue)
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

    if (numChars && prv::MatchFloat(originalPos, outValue, numChars))
        return true;

    m_cur = originalPos;
    return false;
}

bool Parser::parseBoolean(bool& outValue)
{
    if (parseKeyword("true"))
    {
        outValue = true;
        return true;
    }
    else if (parseKeyword("false"))
    {
        outValue = false;
        return true;
    }

    int64_t numericValue = 0;
    if (parseInt64(numericValue))
    {
        outValue = (numericValue != 0);
        return true;
    }

    return false;
}

bool Parser::parseHex(uint64_t& outValue, uint32_t maxLength /*= 0*/, uint32_t* outValueLength /*= nullptr*/)
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

    outValue = ret;
    if (outValueLength)
        *outValueLength = (uint32_t)(m_cur - original);
    return true;
}

bool Parser::parseInt8(char& outValue)
{
    auto start = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<char>::min() || bigVal > std::numeric_limits<char>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (char)bigVal;
    return true;
}

bool Parser::parseInt16(short& outValue)
{
    auto start = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<short>::min() || bigVal > std::numeric_limits<short>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (short)bigVal;
    return true;
}

bool Parser::parseInt32(int& outValue)
{
    auto start = m_cur;

    int64_t bigVal = 0;
    if (!parseInt64(bigVal))
        return false;

    if (bigVal < std::numeric_limits<int>::min() || bigVal > std::numeric_limits<int>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (int)bigVal;
    return true;
}

bool Parser::parseInt64(int64_t& outValue)
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

    if (numChars && prv::MatchInteger(originalPos, outValue, numChars, 10))
        return true;

    m_cur = originalPos;
    return false;
}

bool Parser::parseUint8(uint8_t& outValue)
{
    auto start = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint8_t>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (uint8_t)bigVal;
    return true;
}

bool Parser::parseUint16(uint16_t& outValue)
{
    auto start = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint16_t>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (uint16_t)bigVal;
    return true;
}

bool Parser::parseUint32(uint32_t& outValue)
{
    auto start = m_cur;

    uint64_t bigVal = 0;
    if (!parseUint64(bigVal))
        return false;

    if (bigVal > std::numeric_limits<uint32_t>::max())
    {
        m_cur = start;
        return false;
    }

    outValue = (uint32_t)bigVal;
    return true;
}

bool Parser::parseUint64(uint64_t& outValue)
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

    if (numChars && prv::MatchInteger(originalPos, outValue, numChars, 10))
        return true;

    m_cur = originalPos;
    return false;
}

//--

const std::string& Commandline::get(std::string_view name) const
{
    for (const auto& entry : args)
        if (entry.key == name)
            return entry.value;

    static std::string theEmptyString;
    return theEmptyString;
}

const std::vector<std::string>& Commandline::getAll(std::string_view name) const
{
    for (const auto& entry : args)
        if (entry.key == name)
            return entry.values;

    static std::vector<std::string> theEmptyStringArray;
    return theEmptyStringArray;
}

bool Commandline::has(std::string_view name) const
{
    for (const auto& entry : args)
        if (entry.key == name)
            return true;

    return false;
}

bool Commandline::parse(std::string_view text)
{
    Parser parser(text);

    bool parseInitialChar = true;
    while (parser.parseWhitespaces())
    {
        if (parser.parseKeyword("-"))
        {
            parseInitialChar = false;
            break;
        }

        // get the command
        std::string_view commandName;
        if (!parser.parseIdentifier(commandName))
        {
            std::cout << "Commandline parsing error: expecting command name. Application may not work as expected.\n";
            return false;
        }

        commands.push_back(std::string(commandName));
    }

    while (parser.parseWhitespaces())
    {
        if (!parser.parseKeyword("-") && parseInitialChar)
            break;
        parseInitialChar = true;

        std::string_view paramName;
        if (!parser.parseIdentifier(paramName))
        {
            std::cout << "Commandline parsing error: expecting param name after '-'. Application may not work as expected.\n";
            return false;
        }

        std::string_view paramValue;
        if (parser.parseKeyword("="))
        {
            // Read value
            if (!parser.parseString(paramValue))
            {
                std::cout << "Commandline parsing error: expecting param value after '=' for param '" << paramName << "'. Application may not work as expected.\n";
                return false;
            }
        }

        bool exists = false;
        for (auto& param : this->args)
        {
            if (param.key == paramName)
            {
                if (!paramValue.empty())
                {
                    param.values.push_back(std::string(paramValue));
                    param.value = paramValue;
                }

                exists = true;
                break;
            }
        }

        if (!exists)
        {
            Arg arg;
            arg.key = paramName;

            if (!paramValue.empty())
            {
                arg.values.push_back(std::string(paramValue));
                arg.value = paramValue;
            }

            this->args.push_back(arg);
        }
    }

    return true;
}

//--

bool LoadFileToString(const fs::path& path, std::string& outText)
{
    try
    {
        std::ifstream f(path);
        std::stringstream buffer;
        buffer << f.rdbuf();
        outText = buffer.str();
        return true;
    }
    catch (std::exception& e)
    {
        std::cout << "Error reading file " << path << ": " << e.what() << "\n";
        return false;
    }
}

bool SaveFileFromString(const fs::path& path, std::string_view txt, bool force /*= false*/, uint32_t* outCounter, fs::file_time_type customTime /*= fs::file_time_type()*/)
{
    std::string newContent(txt);

    if (!force)
    {
        std::string currentContent;
        if (LoadFileToString(path, currentContent))
        {
            if (currentContent == txt)
            {
                if (customTime != fs::file_time_type())
                    fs::last_write_time(path, customTime);

                return true;
            }
        }

        std::cout << "File " << path << " has changed and has to be saved\n";
    }

    {
        std::error_code ec;
        fs::create_directories(path.parent_path(), ec);
    }

    try
    {
        std::ofstream file(path);
        file << txt;
    }
    catch (std::exception& e)
    {
        std::cout << "Error writing file " << path << ": " << e.what() << "\n";
        return false;
    }

    if (outCounter)
        (*outCounter) += 1;

    return true;
}

//--

void SplitString(std::string_view txt, std::string_view delim, std::vector<std::string_view>& outParts)
{
    size_t prev = 0, pos = 0;
    do
    {
        pos = txt.find(delim, prev);
        if (pos == std::string::npos) 
            pos = txt.length();

        std::string_view token = txt.substr(prev, pos - prev);
        if (!token.empty()) 
            outParts.push_back(token);

        prev = pos + delim.length();
    }
    while (pos < txt.length() && prev < txt.length());
}

bool BeginsWith(std::string_view txt, std::string_view end)
{
    if (txt.length() >= end.length())
        return (0 == txt.compare(0, end.length(), end));
    return false;
}

bool EndsWith(std::string_view txt, std::string_view end)
{
    if (txt.length() >= end.length())
        return (0 == txt.compare(txt.length() - end.length(), end.length(), end));
    return false;
}

std::string_view PartBefore(std::string_view txt, std::string_view end)
{
    auto pos = txt.find(end);
    if (pos != -1)
        return txt.substr(0, pos);
    return "";
}

std::string_view PartAfter(std::string_view txt, std::string_view end)
{
    auto pos = txt.find(end);
    if (pos != -1)
        return txt.substr(pos + end.length());
    return "";
}

std::string MakeGenericPath(std::string_view txt)
{
    auto ret = std::string(txt);
    std::replace(ret.begin(), ret.end(), '\\', '/');
    return ret;
}

std::string MakeGenericPathEx(const fs::path& path)
{
    return MakeGenericPath(path.u8string());
}

std::string ToUpper(std::string_view txt)
{
    std::string ret(txt);
    transform(ret.begin(), ret.end(), ret.begin(), ::toupper);    
    return ret;
}

void writeln(std::stringstream& s, std::string_view txt)
{
    s << txt;
    s << "\n";
}

void writelnf(std::stringstream& s, const char* txt, ...)
{
    char buffer[8192];
    va_list args;
    va_start(args, txt);
    vsprintf_s(buffer, sizeof(buffer), txt, args);
    va_end(args);

    s << buffer;
    s << "\n";
}

std::string GuidFromText(std::string_view txt)
{
    union {        
        struct {
            uint32_t Data1;
            uint16_t Data2;
            uint16_t Data3;
            uint8_t Data4[8];
        } g;

        uint32_t data[4];
    } guid;

    guid.data[0] = (uint32_t)std::hash<std::string_view>()(txt);
    guid.data[1] = (uint32_t)std::hash<std::string>()("part1_" + std::string(txt));
    guid.data[2] = (uint32_t)std::hash<std::string>()("part2_" + std::string(txt));
    guid.data[3] = (uint32_t)std::hash<std::string>()("part3_" + std::string(txt));

    // 2150E333-8FDC-42A3-9474-1A3956D46DE8

    char str[128];
    sprintf_s(str, sizeof(str), "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
        guid.g.Data1, guid.g.Data2, guid.g.Data3,
        guid.g.Data4[0], guid.g.Data4[1], guid.g.Data4[2], guid.g.Data4[3],
        guid.g.Data4[4], guid.g.Data4[5], guid.g.Data4[6], guid.g.Data4[7]);

    return str;
}

//--

#define MATCH(_txt, _val) if (txt == _txt) { outType = _val; return true; }

template< typename T >
bool ParseEnumValue(std::string_view txt, T& outType)
{
    for (int i = 0; i < (int)(T::MAX); ++i)
    {
        const auto valueName = NameEnumOption((T)i);
        if (!valueName.empty() && valueName == txt)
        {
            outType = (T)i;
            return true;
        }
    }

    return false;
}

bool ParseConfigurationType(std::string_view txt, ConfigurationType& outType)
{
    return ParseEnumValue(txt, outType);
}

bool ParseBuildType(std::string_view txt, BuildType& outType)
{
    return ParseEnumValue(txt, outType);
}

bool ParseLibraryType(std::string_view txt, LibraryType& outType)
{
    return ParseEnumValue(txt, outType);
}

bool ParsePlatformType(std::string_view txt, PlatformType& outType)
{
    return ParseEnumValue(txt, outType);
}

bool ParseGeneratorType(std::string_view txt, GeneratorType& outType)
{
    return ParseEnumValue(txt, outType);
}


//--

std::string_view NameEnumOption(ConfigurationType type)
{
    switch (type)
    {
    case ConfigurationType::Checked: return "checked";
    case ConfigurationType::Debug: return "debug";
    case ConfigurationType::Release: return "release";
    case ConfigurationType::Final: return "final";
    }
    return "";
}

std::string_view NameEnumOption(BuildType type)
{
    switch (type)
    {
    case BuildType::Development: return "dev";
    case BuildType::Shipment: return "ship";
    }
    return "";
}

std::string_view NameEnumOption(LibraryType type)
{
    switch (type)
    {
    case LibraryType::Shared: return "shared";
    case LibraryType::Static: return "static";
    }
    return "";
}

std::string_view NameEnumOption(PlatformType type)
{
    switch (type)
    {
    case PlatformType::Linux: return "linux";
    case PlatformType::Windows: return "windows";
    case PlatformType::UWP: return "uwp";
    }
    return "";
}

std::string_view NameEnumOption(GeneratorType type)
{
    switch (type)
    {
    case GeneratorType::VisualStudio: return "vs2019";
    case GeneratorType::CMake: return "cmake";
    }
    return "";
}

//--

bool IsFileSourceNewer(const fs::path& source, const fs::path& target)
{
    try
    {
        if (!fs::is_regular_file(source))
            return false;

        if (!fs::is_regular_file(target))
            return true;

        auto sourceTimestamp = fs::last_write_time(source);
        auto targetTimestamp = fs::last_write_time(target);
        return sourceTimestamp > targetTimestamp;
    }
    catch (std::exception & e)
    {
        std::cout << "Failed to check file write time: " << e.what() << "\n";
        return false;
    }    
}

bool CopyNewerFile(const fs::path& source, const fs::path& target)
{
    try
    {
        if (!fs::is_regular_file(source))
            return false;

        if (fs::is_regular_file(target))
        {
            auto sourceTimestamp = fs::last_write_time(source);
            auto targetTimestamp = fs::last_write_time(target);
            if (targetTimestamp >= sourceTimestamp)
                return true;
        }

        std::cout << "Copying " << target << "\n";
        fs::remove(target);
        fs::create_directories(target.parent_path());
        fs::copy(source, target);

        return true;
    }
    catch (std::exception & e)
    {
        std::cout << "Failed to copy file: " << e.what() << "\n";
        return false;
    }
}