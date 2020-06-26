/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: string #]
***/

#pragma once

#include "stringBuf.h"

namespace base
{
    /// simple parser
    class BASE_CONTAINERS_API StringParser
    {
    public:
        StringParser();
        StringParser(StringView<char> view);
        StringParser(const void* data, uint32_t size);
        StringParser(const StringParser& other);
        StringParser& operator=(const StringParser& other);
        INLINE ~StringParser() = default;

        // clear to empty state
        void clear();

        // rewind to the start
        void rewind();

        //--

        // get the view of whole text
        INLINE StringView<char> fullView() const { return StringView<char>(m_start, m_end); }

        // get the view of remaining text
        INLINE StringView<char> currentView() const { return StringView<char>(m_cur, m_end); }

        // get the current line
        INLINE uint32_t line() const { return m_line; }

        //--

        // parse whitespace
        bool parseWhitespaces();

        // parse till the end of the line
        bool parseTillTheEndOfTheLine(StringView<char>* outIdent = nullptr);

        //--

        // parse the standard identifier
        bool parseIdentifier(StringView<char>& outIdent);

        // parse a string from the stream, may be in quotes
        // NOTE: if the string contains escaped characters they are de-escaped
        // NOTE: additional delimiting characters may be passed to break on them (good example is ';' in CSV)
        bool parseString(StringView<char>& outValue, const char* additionalDelims = "");

        // parse a text till the end of the line or till one of the additional delims is found
        // NOTE: additional delimiting characters may be passed to break on them (good example is ';' in CSV)
        bool parseLine(StringView<char>& outValue, const char* additionalDelims = "", bool eatLeadingWhitespaces=true);

        // parse the standard identifier
        bool parseIdentifier(StringID& outIdent);

        // parse matching keyword, if the exact text is found in the stream it's advanced, if not a false is returned
        bool parseKeyword(StringView<char> keyword);

        // parse a floating point number in the standard format
        bool parseFloat(float& outValue);

        // parse a double precision floating point number
        bool parseDouble(double& outValue);

        // parse a boolean value, matches true/false but also a numerical value (true if != 0)
        bool parseBoolean(bool& outValue);

        // parse hex value, NOTE: maximum length can be limited (so we can parse byte or 2 etc)
        bool parseHex(uint64_t& outValue, uint32_t maxLength = 0, uint32_t* outValueLength = nullptr);

        // parse int8, if value is out of range we don't parse it
        bool parseInt8(char& outValue);

        // parse int16, if value is out of range we don't parse it
        bool parseInt16(short& outValue);

        // parse int32, if value is out of range we don't parse it
        bool parseInt32(int& outValue);

        // parse int64
        bool parseInt64(int64_t& outValue);

        // parse uint8, if value is out of range we don't parse it
        bool parseUint8(uint8_t& outValue);

        // parse uint16, if value is out of range we don't parse it
        bool parseUint16(uint16_t& outValue);

        // parse uint32, if value is out of range we don't parse it
        bool parseUint32(uint32_t& outValue);

        // parse uint64, if value is out of range we don't parse it
        bool parseUint64(uint64_t& outValue);

        // parse type name
        bool parseTypeName(StringID& outTypeName);

        //--

        // peak at the next keyword, does not advance the state
        bool testKeyword(StringView<char> keyword) const;

        //--

        // push state on stack
        void push();

        // pop state from stack
        void pop();

    private:
        const char* m_start;
        const char* m_cur;
        const char* m_end;
        uint32_t m_line;

        struct State
        {
            const char* cur = nullptr;
            uint32_t line = 0;
        };

        base::Array<State> m_stateStack;
    };

} // base