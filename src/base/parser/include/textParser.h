/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: parser #]
***/

#pragma once

#include "textToken.h"
#include "textLanguageDefinition.h"
#include "textErrorReporter.h"

#include "base/containers/include/inplaceArray.h"
#include "base/memory/include/linearAllocator.h"

BEGIN_BOOMER_NAMESPACE(base::parser)

///----

/// helper class for manual parsing of simple text documents, keeps track of the current line and can report nice errors
/// NOTE: this class is NOT good for parsing a line/indentation based code (ie. Python, etc) as it does not respect line boundaries well
/// NOTE: we only have basic UTF support here
class BASE_PARSER_API TextParser : public NoCopy
{
public:
    TextParser(StringView context = StringView(), IErrorReporter& errorReporter = IErrorReporter::GetDefault(), ICommentEater& commentEater = ICommentEater::NoComments());
    ~TextParser();

    //--

    // get the name of the context we are parsing
    INLINE const StringBuf& contextName() const { return m_context; }

    // get the current line
    INLINE uint32_t line() const { return m_lineIndex; }

    // did we have any errors reporter ?
    INLINE bool hasErrors() const { return m_numErrors != 0; }

    // get number of reported errors
    INLINE uint32_t numErrors() const { return m_numErrors; }

    // get number of reported warnings
    INLINE uint32_t numWarnings() const { return m_numWarnings; }

    // get view on current parsing position
    INLINE StringView view() const { return StringView(m_pos, m_end); }

    //--

    // reset content and rewind
    // NOTE: we can parse a NON ZERO TERMINATED DATA directly
    void reset(StringView data);

    // reset content and rewind
    // NOTE: we can parse a NON ZERO TERMINATED DATA directly
    void reset(const Buffer& buffer);

    // rewind
    void rewind();

    //--

    // report an error to attached error reporter at current location
    // NOTE: this function returns false so it can be chained in one-liner return ctx.error(..)
    bool error(StringView msg);

    // report a warning to attached error reporter at current location
    void warning(StringView msg);

    //--

    // skip whitespaces till something is found, will close line boundaries and skip comments via the comment eater
    // returns false if End Of Document was reached
    bool findNextContent(bool thisLineOnly = false);

    //--

    // parser a specific keyword (any text), if it is not parsed and the isRequired flag is set than an auto matic  error is reported
    bool parseKeyword(const char* str, bool thisLine=false, bool isRequired=false);

    // parse an identifier name, only a-z, A-Z, _ and numbers are allowed, typical identifier name
    bool parseIdentifier(StringView& outIdent, bool thisLine=true, bool isRequired=true, const char* additionalChars = "");

    // parse a double quoted string
    bool parseString(StringView& outText, bool thisLine=true, bool isRequired=true);

    // parse a single quoted string
    bool parseName(StringView& outText, bool thisLine=true, bool isRequired=true);

    // parse a 8 bit unsigned value, out of range numbers are not parsed
    bool parseUint8(uint8_t& outValue, bool thisLine=true, bool isRequired=true);

    // parse a 16 bit unsigned value, out of range numbers are not parsed
    bool parseUint16(uint16_t& outValue, bool thisLine=true, bool isRequired=true);

    // parse a 16 bit unsigned value, out of range numbers are not parsed
    bool parseUint32(uint32_t& outValue, bool thisLine=true, bool isRequired=true);

    // parse a 64 bit unsigned value, out of range numbers are not parsed
    bool parseUint64(uint64_t& outValue, bool thisLine=true, bool isRequired=true);

    // parse a 8 bit signed value, out of range numbers are not parsed
    bool parseInt8(char& outValue, bool thisLine=true, bool isRequired=true);

    // parse a 16 bit signed value, out of range numbers are not parsed
    bool parseInt16(short& outValue, bool thisLine=true, bool isRequired=true);

    // parse a 32 bit signed value, out of range numbers are not parsed
    bool parseInt32(int& outValue, bool thisLine=true, bool isRequired=true);

    // parse a 64 bit signed value, out of range numbers are not parsed
    bool parseInt64(int64_t& outValue, bool thisLine=true, bool isRequired=true);

    // parse a single precession floating point value
    bool parseFloat(float& outValue, bool thisLine=true, bool isRequired=true);

    // parse a single precession floating point value
    bool parseDouble(double& outValue, bool thisLine=true, bool isRequired=true);

    // parse a hex value
    bool parseHex(uint64_t& outValue, uint32_t maxLength = 0, uint32_t* outValueLength = nullptr, bool thisLine=true, bool isRequired=true);

    //--

    // peak a matching keyword (does not advance the stuff)
    bool peekKeyword(const char* str, bool thisLine=false) const;

    //--

    // parse a general token using a language definition, will capture strings and numbers as well
    // NOTE: resulting token will be allocated from memory pool
    Token* parseToken(mem::LinearAllocator& mem, const ILanguageDefinition& language);

    // parse a general token using a language definition, will capture strings and numbers as well
    Token parseToken(const ILanguageDefinition& language);

    //--

    // push state onto the stack, allows for speculative parsing
    void pushState();

    // pop state from the stack
    void popState();

    // discard history state without poping
    void discardState();

private:
    const char* m_start;
    const char* m_end;
    const char* m_lineEnd;
    const char* m_pos;
    uint32_t m_lineIndex;
    uint32_t m_numErrors;
    uint32_t m_numWarnings;

    struct HistoryState
    {
        const char* m_lineEnd;
        const char* m_pos;
        uint32_t m_lineIndex;
    };

    base::InplaceArray<HistoryState, 6> m_stateHistory;

    StringBuf m_context;
    IErrorReporter& m_errorReporter;
    ICommentEater& m_commentEater;
};

//---

END_BOOMER_NAMESPACE(base::parser)