/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: parser #]
***/

#include "build.h"
#include "textToken.h"
#include "textLanguageDefinition.h"

BEGIN_BOOMER_NAMESPACE(base::parser)

namespace prv
{

    //--

    // no comments
    class NoCommentsParserCommentEater : public ICommentEater
    {
    public:
        virtual void eatWhiteSpaces(const char*& str, const char* endStr, uint32_t& lineIndex) const override
        {
            while (str < endStr)
            {
                if (*str > ' ')
                    break;
                if (*str == '\n')
                    lineIndex += 1;
                ++str;
            }
        }
    };

    //--

    // standard C/C++ comments
    class StandardCommentsParserCommentEater : public NoCommentsParserCommentEater
    {
    public:
        virtual void eatWhiteSpaces(const char*& str, const char* endStr, uint32_t& lineIndex) const override
        {
            // eat basic white spaces
            NoCommentsParserCommentEater::eatWhiteSpaces(str, endStr, lineIndex);

            // we need at least 2 chars :)
            if (str + 2 <= endStr)
            {
                // a line comment ?
                if (str[0] == '/' && str[1] == '/')
                {
                    // eat until the end of line or end of file
                    str += 2;
                    while (str < endStr)
                    {
                        if (*str == '\n')
                        {
                            lineIndex += 1; // we've consumed the '\n' so we have to increment the line counter
                            str += 1; // eat the new line
                            break;
                        }
                        ++str;
                    }
                }
                    // a block comment ?
                else if (str[0] == '/' && str[1] == '*')
                {
                    // eat until the end of line or end of file
                    while (str + 1 < endStr)
                    {
                        if (str[0] == '\n')
                            lineIndex += 1; // we've consumed the '\n' so we have to increment the line counter

                        if (str[0] == '*' && str[1] == '/')
                        {
                            str += 2;
                            break;
                        }

                        ++str;
                    }
                }
            }
        }

        /*virtual bool eatPreprocessor(const char*& str, const char* endStr, uint32_t& lineIndex, Token& outToken) const override
        {
            // eat basic white spaces
            NoCommentsParserCommentEater::eatWhiteSpaces(str, endStr, lineIndex);

            // pragma ?
            if (str < endStr)
            {
                if (*str == '#')
                {
                    auto start  = str;
                    auto startLine = lineIndex;
                    str += 1;

                    while (str < endStr)
                    {
                        // advance past the line ending
                        if (*str == '\\' && str + 2 < endStr)
                        {
                            if (str[1] == '\n')
                            {
                                lineIndex += 1;

                                if (str[2] == '\r')
                                    str += 3;
                                else
                                    str += 2;
                            }
                            else if (str[1] == '\r' && str[2] == '\n')
                            {
                                lineIndex += 1;
                                str += 3;
                            }
                        }

                        // end of define
                        if (*str == '\n')
                            break;

                        // keep eating
                        str += 1;
                    }

                    if (str > start)
                        outToken = Token(TextTokenType::Preprocessor, start, str, -1);
                    return true;
                }
            }

            // not a pragma
            return false;
        }*/
    };

    //--

    // standard LUA -- comments
    class LUACommentsParserCommentEater : public NoCommentsParserCommentEater
    {
    public:
        virtual void eatWhiteSpaces(const char*& str, const char* endStr, uint32_t& lineIndex) const override
        {
            // eat basic white spaces
            NoCommentsParserCommentEater::eatWhiteSpaces(str, endStr, lineIndex);

            // we need at least 2 chars :)
            if (str + 2 <= endStr)
            {
                // a line comment ?
                if (str[0] == '-' && str[1] == '-')
                {
                    // eat until the end of line or end of file
                    str += 2;
                    while (str < endStr)
                    {
                        if (*str == '\n')
                        {
                            lineIndex += 1; // we've consumed the '\n' so we have to increment the line counter
                            break;
                        }
                        ++str;
                    }
                }
            }
        }
    };

    //--

    // python-line # commends
    class HashCommentsParserCommentEater : public NoCommentsParserCommentEater
    {
    public:
        virtual void eatWhiteSpaces(const char*& str, const char* endStr, uint32_t& lineIndex) const override
        {
            // we need at least 2 chars :)
            if (str < endStr)
            {
                // a line comment ?
                if (str[0] == '#')
                {
                    // eat until the end of line or end of file
                    str += 1;
                    while (str < endStr)
                    {
                        if (*str == '\n')
                        {
                            lineIndex += 1; // we've consumed the '\n' so we have to increment the line counter
                            break;
                        }
                        ++str;
                    }
                }
            }
        }
    };

    //--

} // prv

//--

ICommentEater::~ICommentEater()
{}

ICommentEater& ICommentEater::NoComments()
{
    static prv::NoCommentsParserCommentEater theEater;
    return theEater;
}

ICommentEater& ICommentEater::StandardComments()
{
    static prv::StandardCommentsParserCommentEater theEater;
    return theEater;
}

ICommentEater& ICommentEater::LuaComments()
{
    static prv::LUACommentsParserCommentEater theEater;
    return theEater;
}

ICommentEater& ICommentEater::HashComments()
{
    static prv::HashCommentsParserCommentEater theEater;
    return theEater;
}

//--

ILanguageDefinition::ILanguageDefinition()
{}

ILanguageDefinition::~ILanguageDefinition()
{}

//--

END_BOOMER_NAMESPACE(base::parser)