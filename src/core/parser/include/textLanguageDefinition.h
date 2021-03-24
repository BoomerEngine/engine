/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: parser #]
***/

#pragma once

#include "core/containers/include/stringBuf.h"

BEGIN_BOOMER_NAMESPACE()

///---

/// comment handler for text parser, allows to skip something we consider a comment
class CORE_PARSER_API ITextCommentEater : public NoCopy
{
public:
    virtual ~ITextCommentEater();

    /// look at content and if it's a comment, eat it, eat all other stuff that you consider a white space
    virtual void eatWhiteSpaces(const char*& str, const char* endStr, uint32_t& lineIndex) const = 0;

    //--

    /// no comments allowed in the file (e.g JSON)
    static ITextCommentEater& NoComments();

    /// a standard // and /**/ commend skipper - most of the files, parses the preprocessor stuff as well
    static ITextCommentEater& StandardComments();

    /// a standard LUA -- comments eater
    static ITextCommentEater& LuaComments();

    /// a standard # based comments
    static ITextCommentEater& HashComments();
};

///----

/// simple language definition for parser
/// NOTE: this is not a uber-general lexer, just a simple one that can handle common set of languages, so stuff like strings and named identifier are always parser auto matically  as they are the same across many languages
class CORE_PARSER_API ITextLanguageDefinition : public NoCopy
{
public:
    ITextLanguageDefinition();
    virtual ~ITextLanguageDefinition();

    /// look at the stream of chars and decide what it is, if nothing is found return false, if something is found it should return true
    /// NOTE: we always try to eat the longest sequence, so for example += will be parser preferably to just +, etc
    virtual bool eatToken(const char*& str, const char* endStr, Token& outToken) const = 0;
};

///---

END_BOOMER_NAMESPACE()
