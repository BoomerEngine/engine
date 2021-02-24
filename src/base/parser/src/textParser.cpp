/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: parser #]
***/

#include "build.h"
#include "textToken.h"
#include "textParser.h"
#include "textErrorReporter.h"
#include "textLanguageDefinition.h"

#include "base/containers/include/stringParser.h"

BEGIN_BOOMER_NAMESPACE(base::parser)

//--

namespace prv
{

    /// a simple implementation of the error reporter that prints the errors in the log
    class LogTextParserErrorReporter : public IErrorReporter
    {
    public:
        LogTextParserErrorReporter()
        {}

        virtual void reportError(const Location& loc, StringView message) override
        {
            TRACE_ERROR("{}: error: {}", loc, message);
        }

        virtual void reportWarning(const Location& loc, StringView message) override
        {
            TRACE_WARNING("{}: warning: {}", loc, message);
        }
    };

    /// a simple implementation of the error reporter that prints the errors in the log
    class DevNullErrorReporter : public IErrorReporter
    {
    public:
        DevNullErrorReporter()
        {}

        virtual void reportError(const Location& loc, StringView message) override
        {
        }

        virtual void reportWarning(const Location& loc, StringView message) override
        {
        }
    };

    class NoIncludesHandler : public IIncludeHandler
    {
    private:
        virtual bool loadInclude(bool global, StringView path, StringView referencePath, Buffer& outContent, StringBuf& outPath) override final
        {
            return false;
        }
    };

} // prv

//--

IErrorReporter::~IErrorReporter()
{}

IErrorReporter& IErrorReporter::GetDefault()
{
    static prv::LogTextParserErrorReporter theReporter;
    return theReporter;
}

IErrorReporter& IErrorReporter::GetDevNull()
{
    static prv::DevNullErrorReporter theReporter;
    return theReporter;            
}

//--

IIncludeHandler::~IIncludeHandler()
{}

IIncludeHandler& IIncludeHandler::GetEmptyHandler()
{
    static prv::NoIncludesHandler theHandler;
    return theHandler;
}

//--

TextParser::TextParser(StringView context /*= StringBuf::EMPTY()*/, IErrorReporter& errorReporter /*= IErrorReporter::GetDefault()*/, ICommentEater& commentEater /*= ICommentEater::NoComments()*/)
    : m_start(nullptr)
    , m_end(nullptr)
    , m_lineEnd(nullptr)
    , m_pos(nullptr)
    , m_lineIndex(1)
    , m_numErrors(0)
    , m_numWarnings(0)
    , m_context(context)
    , m_errorReporter(errorReporter)
    , m_commentEater(commentEater)
{}

void TextParser::pushState()
{
    auto& state = m_stateHistory.emplaceBack();
    state.m_lineIndex = m_lineIndex;
    state.m_lineEnd = m_lineEnd;
    state.m_pos = m_pos;
}

void TextParser::popState()
{
    ASSERT_EX(!m_stateHistory.empty(), "Parser state history is empty");

    auto& state = m_stateHistory.back();
    m_lineIndex = state.m_lineIndex;
    m_pos = state.m_pos;
    m_lineEnd = state.m_lineEnd;

    m_stateHistory.popBack();
}

void TextParser::discardState()
{
    ASSERT_EX(!m_stateHistory.empty(), "Parser state history is empty");
    m_stateHistory.popBack();
}

TextParser::~TextParser()
{}

void TextParser::reset(StringView data)
{
    m_start = data.data();
    m_end = data.data() + data.length();
    rewind();
}

void TextParser::reset(const Buffer& buffer)
{
    if (buffer)
    {
        m_start = (const char *) buffer.data();
        m_end = m_start + buffer.size();
    }
    else
    {
        m_start = nullptr;
        m_end = nullptr;
    }
    rewind();
}

void TextParser::rewind()
{
    m_pos = m_start;
    m_lineIndex = 1;
    m_numErrors = 0;
    m_numWarnings = 0;
}

bool TextParser::error(StringView txt)
{
    Location location(m_context, m_lineIndex);
    m_errorReporter.reportError(location, txt);
    return false;
}

void TextParser::warning(StringView txt)
{
    Location location(m_context, m_lineIndex);
    m_errorReporter.reportWarning(location, txt);
}

bool TextParser::findNextContent(bool thisLineOnly)
{
    while (m_pos < m_end)
    {
        auto newPos = m_pos;
        auto newLineIndex = m_lineIndex;
        m_commentEater.eatWhiteSpaces(newPos, m_end, newLineIndex);

        if (thisLineOnly && newLineIndex != m_lineIndex)
            return false;

        if (newPos == m_pos)
            break;

        m_pos = newPos;
        m_lineIndex = newLineIndex;
    }

    return m_pos < m_end;
}

bool TextParser::peekKeyword(const char* str, bool thisLine) const
{
    auto pos  = m_pos;
    while (pos < m_end)
    {
        auto newPos = pos;
        auto newLineIndex = m_lineIndex;
        m_commentEater.eatWhiteSpaces(newPos, m_end, newLineIndex);

        if (thisLine && newLineIndex != m_lineIndex)
            return false;

        if (newPos == pos)
            break;

        pos = newPos;
    }

    // get the length of the string to match
    auto length = strlen(str);
    if (pos + length > m_end)
        return false;

    // is this the string we are looking for ?
    if (0 != strncmp(pos, str, length))
        return false;

    // we have the keyword
    return true;
}

bool TextParser::parseKeyword(const char* str, bool thisLine/*=true*/, bool isRequired/*=false*/)
{
    // go to content
    if (!findNextContent(thisLine))
        return false;

    // get the length of the string to match
    auto length = strlen(str);
    if (m_pos + length > m_end)
        return isRequired ? error(TempString("Expected keyword '{}', found end of file", str)) : false;

    // is this the string we are looking for ?
    if (0 != strncmp(m_pos, str, length))
        return isRequired ? error(TempString("Missing required keyword '{}'", str)) : false;

    // advance
    m_pos += length;
    return true;
}

static bool IsValidIdentChar(char ch, bool first, const char* additionalChars)
{
    if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_')
        return true;

    if (!first && (ch >= '0' && ch <= '9'))
        return true;

    if (!first && strchr(additionalChars, ch))
        return true;

    return false;
}

bool TextParser::parseIdentifier(StringView& outIdent, bool thisLine/*=true*/, bool isRequired/*=true*/, const char* additionalChars /*""*/)
{
    // go to content
    if (!findNextContent(thisLine))
        return false;

    // get the chars till the non alpha num
    auto cur  = m_pos;
    while (cur < m_end)
    {
        if (!IsValidIdentChar(*cur, cur == m_pos, additionalChars))
            break;
        ++cur;
    }

    // parsed anything ?
    if (cur > m_pos)
    {
        outIdent = StringView(m_pos, cur - m_pos);
        m_pos = cur;
        return true;
    }

    // not parsed
    return isRequired ? error("Missing required identifier") : false;
}

bool TextParser::parseString(StringView& outText, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    // go to content
    if (!findNextContent(thisLine))
        return false;

    // we need to have a quotes
    auto cur  = m_pos;
    if (*cur == '\"')
    {
        cur += 1;
        auto start  = cur;
        while (cur < m_end)
        {
            if (*cur++ == '\"')
            {
                outText = StringView(start, cur - start - 1);
                m_pos = cur;
                return true;
            }
        }
    }

    // not parsed
    return isRequired ? error("Missing required string") : false;
}

bool TextParser::parseName(StringView& outText, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    // go to content
    if (!findNextContent(thisLine))
        return false;

    // we need to have a quotes
    auto cur  = m_pos;
    if (*cur == '\'')
    {
        cur += 1;
        auto start  = cur;
        while (cur < m_end)
        {
            if (*cur++ == '\'')
            {
                outText = StringView(start, cur - start - 1);
                m_pos = cur;
                return true;
            }
        }
    }

    // not parsed
    return isRequired ? error("Missing required name") : false;
}

bool TextParser::parseHex(uint64_t& outValue, uint32_t maxLength /*= 0*/, uint32_t* outValueLength/*= nullptr*/, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseHex(outValue, maxLength, outValueLength))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected hexadecimal unsigned integer") : false;
    }
}

bool TextParser::parseUint8(uint8_t& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseUint8(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected 8-bit unsigned integer") : false;
    }
}

bool TextParser::parseUint16(uint16_t& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseUint16(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected 16-bit unsigned integer") : false;
    }
}

bool TextParser::parseUint32(uint32_t& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseUint32(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected 32-bit unsigned integer") : false;
    }
}

bool TextParser::parseUint64(uint64_t& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseUint64(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected 64-bit unsigned integer") : false;
    }
}

bool TextParser::parseInt8(char& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseInt8(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected 8-bit signed integer") : false;
    }
}

bool TextParser::parseInt16(short& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
        if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseInt16(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected 16-bit signed integer") : false;
    }
}

bool TextParser::parseInt32(int& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseInt32(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected 32-bit signed integer") : false;
    }
}

bool TextParser::parseInt64(int64_t& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseInt64(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected 64-bit signed integer") : false;
    }
}

bool TextParser::parseFloat(float& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseFloat(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected floating point number") : false;
    }
}

bool TextParser::parseDouble(double& outValue, bool thisLine/*=true*/, bool isRequired/*=true*/)
{
    if (!findNextContent(thisLine))
        return false;

    StringParser parser(StringView(m_pos, m_end));
    if (parser.parseDouble(outValue))
    {
        m_pos = parser.currentView().data();
        return true;
    }
    else
    {
        return isRequired ? error("Expected floating point number") : false;
    }
}

static uint32_t CountCharsTillLineStart(const char* pos, const char* docStart)
{
    auto start  = pos;

    while (pos > docStart)
    {
        if (*pos == '\r' || *pos == '\n')
        {
            pos += 1;
            break;
        }
        --pos;
    }

    return start - pos;
}

Token* TextParser::parseToken(mem::LinearAllocator& mem, const ILanguageDefinition& language)
{
    auto curLineIndex = m_lineIndex;

    // parse
    auto cur  = m_pos;
    auto token  = mem.createNoCleanup<Token>();
    if (!language.eatToken(cur, m_end, *token))
        return nullptr;

    // return parsed text
    m_pos = cur;
    auto charPos = CountCharsTillLineStart(token->view().data(), m_start);
    token->assignLocation(Location(m_context, curLineIndex, charPos));
    return token;
}

Token TextParser::parseToken(const ILanguageDefinition& language)
{
    // go to content
    if (!findNextContent())
        return Token();

    auto curLineIndex = m_lineIndex;

    // parse
    auto cur  = m_pos;
    Token token;
    if (!language.eatToken(cur, m_end, token))
        return Token();

    // return parsed text
    m_pos = cur;
    auto charPos = CountCharsTillLineStart(token.view().data(), m_start);
    token.assignLocation(Location(m_context, curLineIndex, charPos));
    return std::move(token);
}

END_BOOMER_NAMESPACE(base::parser)