/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
***/

#pragma once

#include "core_parser_glue.inl"

BEGIN_BOOMER_NAMESPACE()

///----

/// type of token found
enum class TextTokenType : uint8_t
{
    Invalid=0,

    String, // double quoted string
    Name, // single quoted string
    IntNumber, // an integer numerical value, NOTE: no sign is allowed, may end with "i" or "I"
    UnsignedNumber, // an unsigned numerical value, we know because it ended with "u" or "U"
    FloatNumber, // a floating point numerical value, NOTE: no sign is allowed, may end with "f"
    Char, // a single character from table of simple characters, usually used for {} [] () , + - etc
    Keyword, // a recognized keyword, ie. function, shader, etc whatever was registered in the language
    Identifier, // an identifier (a-z, A-Z, 0-9, _)
    Preprocessor, // a content of perprocessor line, usually beings with # and is eatean till the end
};

///----

class TextTokenLocation;
class Token;
class TokenList;

class ITextCommentEater;
class ITextLanguageDefinition;
class ITextErrorReporter;

class SimpleLanguageDefinitionBuilder;

class TextParser;

//-----

/// error reporter for the text parser
class CORE_PARSER_API ITextErrorReporter : public NoCopy
{
public:
    virtual ~ITextErrorReporter();
    virtual void reportError(const TextTokenLocation& loc, StringView message) = 0;
    virtual void reportWarning(const TextTokenLocation& loc, StringView message) = 0;

    //--

    static ITextErrorReporter& GetDevNull();
    static ITextErrorReporter& GetDefault();
};

//-----

// a handler for including files
class CORE_PARSER_API ITextIncludeHandler : public NoCopy
{
public:
    virtual ~ITextIncludeHandler();

    //----

    // load content of local include
    // "global" include in the <> not ""
    // path - requested path
    // referencePath - path of the file we are calling the #include from 
    // outContent - produced content of the file
    // outPath - reference path for extracted text
    virtual bool loadInclude(bool global, StringView path, StringView referencePath, Buffer& outContent, StringBuf& outPath) = 0;


    //--

    /// get the default "NoIncludes" handler
    static ITextIncludeHandler& GetEmptyHandler();
};

//-----

END_BOOMER_NAMESPACE()
