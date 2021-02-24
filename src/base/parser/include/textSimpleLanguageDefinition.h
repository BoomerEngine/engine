/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [#filter: parser #]
***/

#pragma once

#include "base/containers/include/stringBuf.h"

BEGIN_BOOMER_NAMESPACE(base::parser)

//---

/// helper class to build a language definition based on list of recognized keywords
class BASE_PARSER_API SimpleLanguageDefinitionBuilder : public base::NoCopy
{
public:
    SimpleLanguageDefinitionBuilder();

    //---

    /// clear all stuff
    void clear();

    /// set chars allowed for first and next identifier character
    /// first chars: by default this is _a-zA-Z
    /// next chars: by default this is _a-zA-Z0-9
    /// NOTE: this is not a regex and all allowed chars should be listed explicitly
    /// NOTE: if both arrays are cleared than idents are not allowed
    void identChars(const char* firstChars, const char* nextChars);

    /// enable/disable parsing of integer numbers
    /// NOTE: if integer numbers are disabled but floating point numbers are not disabled than all numbers must be floating point (have a dot)
    void enableIntegerNumbers(bool enabled);

    /// enable/disable parsing of floating point numbers
    void enableFloatNumbers(bool enabled);

    /// enable/disable parsing of double quoted strings
    void enableStrings(bool enabled);

    /// enable/disable parsing of single quoted strings (names)
    void enableNames(bool enabled);

    /// enable/disable treating strings as names
    void enableStringsAsNames(bool enabled);

    /// add a recognized single char that should be reported as char not a keyword
    /// NOTE: chars can also be added as keywords but this is part of the bison legacy to report them like that
    void addChar(char ch);

    /// add a simple keyword
    void addKeyword(const char* txt, int id);

    /// add a preprocessor word
    void addPreprocessor(const char* txt);

    //--

    /// extract a language definition
    UniquePtr<ILanguageDefinition> buildLanguageDefinition() const;

private:
    struct Keyword
    {
        StringBuf m_text;
        int m_id;
        bool m_preprocesor;
    };

    Array<char> m_firstIdentChars;
    Array<char> m_nextIdentChars;
    Array<char> m_chars;
    Array<Keyword> m_keywords;

    bool m_enableInts;
    bool m_enableFloats;
    bool m_enableStrings;
    bool m_enableNames;
    bool m_enableStringsAsNames;
};

//---

END_BOOMER_NAMESPACE(base::parser)